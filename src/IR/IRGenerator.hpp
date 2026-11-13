#pragma once
#include "AST.hpp"
#include "ASTBaseVisitor.hpp"
#include "SemanticAnalyzer.hpp"
#include "SymbolCollector.hpp"
#include <any>
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

enum class llvmTypeKind {
  I32,
  I8,
  I1,
  Ptr,
  Void,
  Label,
  Array,
  Struct,
};

class IRVisitor;
struct Module {
  virtual ~Module() = default;
  virtual std::any accept(IRVisitor *visitor) = 0;
};

struct TypeModule : public Module {
  llvmTypeKind kind;
  bool isSigned = false;
  std::optional<std::size_t> length = std::nullopt;
  std::optional<std::string> structName = std::nullopt;
  std::unique_ptr<TypeModule> innerType = nullptr;
  TypeModule(llvmTypeKind kind) : kind(kind) {}
  TypeModule(TypeModule &&other) {
    kind = other.kind;
    isSigned = other.isSigned;
    length = other.length;
    structName = other.structName;
    innerType = std::move(other.innerType);
  }
  TypeModule(const TypeModule &other) {
    kind = other.kind;
    isSigned = other.isSigned;
    length = other.length;
    structName = other.structName;
    if (other.innerType) {
      innerType = std::make_unique<TypeModule>(*other.innerType);
    } else {
      innerType = nullptr;
    }
  }
  TypeModule &operator=(const TypeModule &other) {
    if (&other != this) {
      kind = other.kind;
      isSigned = other.isSigned;
      length = other.length;
      structName = other.structName;
      if (other.innerType) {
        innerType = std::make_unique<TypeModule>(*other.innerType);
      } else {
        innerType = nullptr;
      }
    }
    return *(this);
  }
  std::any accept(IRVisitor *visitor) override;
  static TypeModule lowerType(const Type *typePtr, bool isReturnType) {
    switch (typePtr->kind) {
    case TypeKind::I32: {
      auto type = TypeModule(llvmTypeKind::I32);
      type.isSigned = true;
      return type;
    }
    case TypeKind::U32: {
      auto type = TypeModule(llvmTypeKind::I32);
      type.isSigned = false;
      return type;
    }
    case TypeKind::Isize: {
      auto type = TypeModule(llvmTypeKind::I32);
      type.isSigned = true;
      return type;
    }
    case TypeKind::Usize: {
      auto type = TypeModule(llvmTypeKind::I32);
      type.isSigned = false;
      return type;
    }
    case TypeKind::Bool: {
      return TypeModule(llvmTypeKind::I8);
    }
    case TypeKind::Unit: {
      if (!isReturnType) {
        auto type = TypeModule(llvmTypeKind::Struct);
        type.structName.emplace("builtin_Unit");
        return type;
      } else {
        auto type = TypeModule(llvmTypeKind::Void);
        return type;
      }
    }
    case TypeKind::MutReference:
    case TypeKind::ImmutReference: {
      return TypeModule(llvmTypeKind::Ptr);
    }
    case TypeKind::Vec: {
      auto type = TypeModule(llvmTypeKind::Struct);
      type.structName.emplace("builtin_Vec");
      return type;
    }
    case TypeKind::Box: {
      return TypeModule(llvmTypeKind::Ptr);
    }
    case TypeKind::Array: {
      auto type = TypeModule(llvmTypeKind::Array);
      type.length = std::get<long long>(typePtr->arrayLen.value().value);
      type.innerType = std::make_unique<TypeModule>(
          lowerType(typePtr->elementType.get(), false));
      return type;
    }
    case TypeKind::Struct: {
      auto type = TypeModule(llvmTypeKind::Struct);
      type.structName.emplace(typePtr->structName.value());
      return type;
    }
    default:
      std::unreachable();
    }
  }
};

struct StructModule : public Module {
  std::string structName;
  std::vector<std::unique_ptr<TypeModule>> fields;
  StructModule() = default;
  StructModule(StructSymbol *symbol) {
    structName = symbol->name;
    for (const auto &fieldName : symbol->layoutFields) {
      const auto &fieldType = symbol->structFields.at(fieldName);
      fields.emplace_back(std::make_unique<TypeModule>(
          TypeModule::lowerType(&fieldType, false)));
    }
  }
  std::any accept(IRVisitor *visitor) override;
};

struct Operand {
  struct SSA {
    std::string SSAName;
    SSA() = default;
    SSA(const std::string &name) : SSAName(name) {}
    SSA &operator=(const std::string &name) {
      SSAName = name;
      return *this;
    }
    SSA(const Operand &operand) { *this = std::get<SSA>(operand.operand); }
  };
  struct Immediate {
    std::variant<long long, bool> constValue;
    Immediate(long long value) : constValue(value) {}
    Immediate(std::variant<long long, bool> &&value) : constValue(value) {}
  };
  struct Unit {};
  std::variant<SSA, Immediate, Unit> operand;
  Operand(long long value) : operand(Immediate(value)) {}
  Operand(const SSA &ssa) : operand(ssa) {};
  Operand(const Immediate &immediate) : operand(immediate) {};
  Operand(Unit unit) : operand(unit) {};
  std::any accept(IRVisitor *visitor);
};

struct Instruction {
  virtual ~Instruction() = default;
  virtual std::any accept(IRVisitor *visitor) = 0;
};

struct BasicBlock;

struct PhiInst : public Instruction {
  std::unique_ptr<TypeModule> type;
  Operand::SSA resultSSA;
  std::vector<std::pair<Operand, BasicBlock *>> incoming;
  PhiInst(const Operand::SSA &result, std::unique_ptr<TypeModule> &&type,
          std::vector<std::pair<Operand, BasicBlock *>> &&incoming)
      : type(std::move(type)), resultSSA(result),
        incoming(std::move(incoming)) {}
  std::any accept(IRVisitor *visitor) override;
};

struct AllocaInst : public Instruction {
  Operand::SSA ptrSSA;
  std::unique_ptr<TypeModule> allocaType;
  AllocaInst(const Operand::SSA &ptr, std::unique_ptr<TypeModule> &&type)
      : ptrSSA(ptr), allocaType(std::move(type)) {}
  std::any accept(IRVisitor *visitor) override;
};

struct LoadInst : public Instruction {
  std::unique_ptr<TypeModule> loadType;
  Operand::SSA ptrSSA;
  Operand::SSA valueSSA;
  LoadInst(std::unique_ptr<TypeModule> &&type, const Operand::SSA &ptr,
           const Operand::SSA &value)
      : loadType(std::move(type)), ptrSSA(ptr), valueSSA(value) {}
  std::any accept(IRVisitor *visitor) override;
};

struct StoreInst : public Instruction {
  std::unique_ptr<TypeModule> storeType;
  Operand storeValue;
  Operand::SSA ptrSSA;
  std::any accept(IRVisitor *visitor) override;
  StoreInst(std::unique_ptr<TypeModule> &&type, const Operand &operand,
            const Operand::SSA &ptr)
      : storeType(std::move(type)), storeValue(operand), ptrSSA(ptr) {}
};

enum class BinaryIntergerOperator {
  Add,
  Sub,
  Mul,
  UDiv,
  SDiv,
  URem,
  SRem,
  Shl,
  LShr,
  AShr,
  And,
  Or,
  Xor,
};
struct BinaryInst : public Instruction {
  BinaryIntergerOperator opKind;
  std::unique_ptr<TypeModule> type;
  Operand leftChild;
  Operand rightChild;
  Operand::SSA resultPtr;
  explicit BinaryInst(BinaryIntergerOperator kind,
                      std::unique_ptr<TypeModule> &&type, const Operand &left,
                      const Operand &right, const Operand::SSA &storePtr)
      : opKind(kind), type(std::move(type)), leftChild(left), rightChild(right),
        resultPtr(storePtr) {}
  std::any accept(IRVisitor *visitor) override;
};
enum class ICmpOperator {
  Eq,
  Ne,
  Ugt,
  Uge,
  Ult,
  Ule,
  Sgt,
  Sge,
  Slt,
  Sle,
};
struct ICmpInst : public Instruction {
  ICmpOperator opKind;
  std::unique_ptr<TypeModule> cmpType;
  Operand leftChild;
  Operand rightChild;
  Operand::SSA resultPtr;
  explicit ICmpInst(ICmpOperator kind, std::unique_ptr<TypeModule> &&type,
                    const Operand &left, const Operand &right,
                    const Operand::SSA &storePtr)
      : opKind(kind), cmpType(std::move(type)), leftChild(left),
        rightChild(right), resultPtr(storePtr) {}
  std::any accept(IRVisitor *visitor) override;
};
enum class CastOperator {
  Trunc,
  ZExt,
  SExt,
};
struct CastInst : public Instruction {
  CastOperator opKind;
  std::unique_ptr<TypeModule> fromType;
  std::unique_ptr<TypeModule> toType;
  Operand castObj;
  Operand::SSA resultPtr;
  explicit CastInst(CastOperator kind, std::unique_ptr<TypeModule> &&from,
                    std::unique_ptr<TypeModule> &&to, const Operand &object,
                    const Operand::SSA &ssa)
      : opKind(kind), fromType(std::move(from)), toType(std::move(to)),
        castObj(object), resultPtr(ssa) {}
  std::any accept(IRVisitor *visitor) override;
};

struct CallInst : public Instruction {
  std::unique_ptr<TypeModule> resultType;
  std::optional<Operand::SSA> resultPtr;
  std::string calleeName;
  std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> params;
  explicit CallInst(
      std::unique_ptr<TypeModule> &&resultType, const Operand::SSA &resultPtr,
      const std::string &calleeName,
      std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> &&params)
      : resultType(std::move(resultType)), resultPtr(resultPtr),
        calleeName(calleeName), params(std::move(params)) {}
  explicit CallInst(
      std::unique_ptr<TypeModule> &&resultType, const std::string &calleeName,
      std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> &&params)
      : resultType(std::move(resultType)), resultPtr(std::nullopt),
        calleeName(calleeName), params(std::move(params)) {}
  std::any accept(IRVisitor *visitor) override;
};

struct GEPInst : public Instruction {
  Operand::SSA resultPtr;
  std::unique_ptr<TypeModule> getObjType;
  Operand::SSA initialPtr;
  std::vector<Operand> indexOfElements;
  GEPInst(const Operand::SSA &result, std::unique_ptr<TypeModule> &&objectType,
          const Operand::SSA &initial, std::vector<Operand> &&indices)
      : resultPtr(result), getObjType(std::move(objectType)),
        initialPtr(initial), indexOfElements(std::move(indices)) {}
  std::any accept(IRVisitor *visitor) override;
};

struct InsertvalueInst : public Instruction {
  Operand::SSA resultPtr;
  std::unique_ptr<TypeModule> aggregateType;
  Operand::SSA aggregatePtr;
  std::unique_ptr<TypeModule> newElementType;
  Operand newElementValue;
  std::vector<std::size_t> indexOfElements;
  InsertvalueInst(const Operand::SSA &result,
                  std::unique_ptr<TypeModule> &&aggregate,
                  const Operand::SSA &aggregateValue,
                  std::unique_ptr<TypeModule> &&elementType,
                  const Operand &elementValue,
                  std::vector<std::size_t> &&indices)
      : resultPtr(result), aggregateType(std::move(aggregate)),
        aggregatePtr(aggregateValue), newElementType(std::move(elementType)),
        newElementValue(elementValue), indexOfElements(std::move(indices)) {}
  std::any accept(IRVisitor *visitor) override;
};

struct ExtractvalueInst : public Instruction {
  Operand::SSA resultPtr;
  std::unique_ptr<TypeModule> aggregateType;
  Operand aggregateValue;
  std::vector<std::size_t> indexOfElements;
  ExtractvalueInst(const Operand::SSA &result,
                   std::unique_ptr<TypeModule> &&aggregate,
                   const Operand &aggregateValue,
                   std::vector<std::size_t> &&indices)
      : resultPtr(result), aggregateType(std::move(aggregate)),
        aggregateValue(aggregateValue), indexOfElements(std::move(indices)) {}
  std::any accept(IRVisitor *visitor) override;
};

struct Terminator : Instruction {
  virtual ~Terminator() = default;
};

struct BasicBlock {
  std::string blockName;
  std::vector<std::unique_ptr<PhiInst>> phis;
  std::vector<std::unique_ptr<Instruction>> insts;
  std::unique_ptr<Terminator> terminator;
  explicit BasicBlock(const std::string &name) : blockName(name) {}
  std::any accept(IRVisitor *visitor);
};

struct BrInst : Terminator {
  BasicBlock *jumpTarget;
  explicit BrInst(BasicBlock *jumpTarget) : jumpTarget(jumpTarget) {}
  std::any accept(IRVisitor *visitor) override;
};

struct CondInst : Terminator {
  Operand cond;
  BasicBlock *trueTarget;
  BasicBlock *falseTarget;
  CondInst(const Operand &cond, BasicBlock *trueTarget, BasicBlock *falseTarget)
      : cond(cond), trueTarget(trueTarget), falseTarget(falseTarget) {}
  std::any accept(IRVisitor *visitor) override;
};

struct ReturnInst : Terminator {
  std::unique_ptr<TypeModule> returnType;
  std::optional<Operand> returnValue;
  std::any accept(IRVisitor *visitor) override;
  ReturnInst(std::unique_ptr<TypeModule> &&returnType)
      : returnType(std::move(returnType)) {}
  ReturnInst(std::unique_ptr<TypeModule> &&returnType,
             const Operand &returnValue)
      : returnType(std::move(returnType)), returnValue(returnValue) {}
};

enum class functionState {
  Declared,
  Defined,
};
struct FunctionModule : public Module {
  FunctionSymbol *symbol = nullptr;
  functionState state;
  std::string functionName;
  std::unique_ptr<TypeModule> returnType;
  std::vector<std::pair<std::unique_ptr<TypeModule>, Operand::SSA>> paramTypes;
  std::unordered_map<std::string, std::unique_ptr<BasicBlock>> basicBlocks;
  BasicBlock *entry = nullptr;
  FunctionModule(FunctionSymbol *symbol)
      : symbol(symbol), state(functionState::Declared),
        functionName(symbol->mangledName),
        returnType(std::make_unique<TypeModule>(
            TypeModule::lowerType(&symbol->returnType, true))) {
    paramTypes.reserve(symbol->paramTypes.size());
    for (const auto &type : symbol->paramTypes) {
      paramTypes.emplace_back(
          std::make_unique<TypeModule>(TypeModule::lowerType(&type, false)),
          Operand::SSA{});
    }
  }
  FunctionModule(
      functionState state, const std::string &funcName,
      std::unique_ptr<TypeModule> &&returnType,
      std::vector<std::pair<std::unique_ptr<TypeModule>, Operand::SSA>>
          &&paramTypes)
      : state(state), functionName(funcName), returnType(std::move(returnType)),
        paramTypes(std::move(paramTypes)) {}
  std::any accept(IRVisitor *visitor) override;
};

struct IRModules : public Module {
  std::string targetTriple = "riscv32-unknown-elf";
  std::string dataLayout = "e-m:e-p:32:32-i64:64-n32-S128";
  std::vector<std::unique_ptr<StructModule>> structs;
  std::unordered_map<std::string, StructModule *> structModuleMap;
  std::vector<std::unique_ptr<FunctionModule>> functions;
  std::unordered_map<const FunctionSymbol *, FunctionModule *>
      functionModuleMap;
  const SemanticAnalyzer *analyzer;
  std::any accept(IRVisitor *visitor) override;
  IRModules(const SemanticAnalyzer *analyzer) : analyzer(analyzer) {
    auto registerExternalFunction = [&](const std::string &name) {
      auto *symbol = analyzer->builtinFunctions.at(name).get();
      auto *module = functions.back().get();
      module->symbol = symbol;
      functionModuleMap.emplace(symbol, module);
    };
    functions.emplace_back(std::make_unique<FunctionModule>(
        functionState::Declared, "get_i32",
        std::make_unique<TypeModule>(llvmTypeKind::I32),
        std::vector<std::pair<std::unique_ptr<TypeModule>, Operand::SSA>>()));
    registerExternalFunction("get_i32");

    std::vector<std::pair<std::unique_ptr<TypeModule>, Operand::SSA>>
        printParams;
    printParams.emplace_back(std::make_unique<TypeModule>(llvmTypeKind::I32),
                             Operand::SSA{});
    functions.emplace_back(std::make_unique<FunctionModule>(
        functionState::Declared, "print_i32",
        std::make_unique<TypeModule>(llvmTypeKind::Void),
        std::move(printParams)));
    registerExternalFunction("print_i32");

    std::vector<std::pair<std::unique_ptr<TypeModule>, Operand::SSA>>
        printlnParams;
    printlnParams.emplace_back(std::make_unique<TypeModule>(llvmTypeKind::I32),
                               Operand::SSA{});
    functions.emplace_back(std::make_unique<FunctionModule>(
        functionState::Declared, "println_i32",
        std::make_unique<TypeModule>(llvmTypeKind::Void),
        std::move(printlnParams)));
    registerExternalFunction("println_i32");

    std::vector<std::pair<std::unique_ptr<TypeModule>, Operand::SSA>>
        allocParams;
    allocParams.emplace_back(std::make_unique<TypeModule>(llvmTypeKind::I32),
                             Operand::SSA{});
    allocParams.emplace_back(std::make_unique<TypeModule>(llvmTypeKind::I32),
                             Operand::SSA{});
    functions.emplace_back(std::make_unique<FunctionModule>(
        functionState::Declared, "__rx_alloc",
        std::make_unique<TypeModule>(llvmTypeKind::Ptr),
        std::move(allocParams)));

    auto builtinUnitStruct = std::make_unique<StructModule>();
    builtinUnitStruct->structName = "builtin_Unit";
    structs.emplace_back(std::move(builtinUnitStruct));
    structModuleMap.emplace(structs.back()->structName, structs.back().get());
    auto builtinVecStruct = std::make_unique<StructModule>();
    builtinVecStruct->structName = "builtin_Vec";
    builtinVecStruct->fields.emplace_back(
        std::make_unique<TypeModule>(llvmTypeKind::Ptr));
    builtinVecStruct->fields.emplace_back(
        std::make_unique<TypeModule>(llvmTypeKind::I32));
    builtinVecStruct->fields.emplace_back(
        std::make_unique<TypeModule>(llvmTypeKind::I32));
    structs.emplace_back(std::move(builtinVecStruct));
    structModuleMap.emplace(structs.back()->structName, structs.back().get());
    addRxItems(analyzer);
  }
  void addRxItems(const SemanticAnalyzer *);
};

enum class SSAType {
  Value,
  Addr,
};

struct FunctionConstextRecord {
  std::size_t addrCounter = 0;
  std::size_t valueCounter = 0;
  std::size_t basicBlockCounter = 0;
  std::unordered_map<const LocalSymbol *, Operand::SSA> localAddressMap;
  FunctionConstextRecord() = default;
  Operand::SSA makeSSA(const std::string &name, SSAType kind) {
    const bool isAddress = kind == SSAType::Addr;
    auto &counter = isAddress ? addrCounter : valueCounter;
    return Operand::SSA{"_" + name + std::to_string(counter++) +
                        (isAddress ? ".addr" : ".value")};
  }
  Operand::SSA makeSSA(SSAType kind) {
    const bool isAddress = kind == SSAType::Addr;
    auto &counter = isAddress ? addrCounter : valueCounter;
    return Operand::SSA{"__" + std::to_string(counter++) +
                        (isAddress ? ".addr" : ".value")};
  }
  std::string generateBasicBlockName() {
    return "BasicBlock." + std::to_string(basicBlockCounter++);
  }
  void clear() {
    addrCounter = 0;
    valueCounter = 0;
    basicBlockCounter = 0;
    localAddressMap.clear();
  }
};

struct LoopContextRecord {
  BasicBlock *continueTarget = nullptr;
  BasicBlock *breakTarget = nullptr;
  std::optional<std::vector<std::pair<Operand, BasicBlock *>>> phiBlocks{};
  bool willExit = false;
};

struct TypeLayout {
  std::size_t size;
  std::size_t alignment;
};
class IRGenerator : public ASTBaseVisitor {
private:
  IRModules *modules = nullptr;
  FunctionConstextRecord guard;
  BasicBlock *currentBlock = nullptr;
  FunctionModule *currentFunction = nullptr;
  std::deque<LoopContextRecord> loopContext;
  std::deque<FunctionModule *> pendingBuiltinFunctions;
  std::unordered_map<std::string, const Type &> pendingEqualFunctions;
  FunctionModule *getOrCreateFunction(FunctionSymbol *symbol);
  void beginFunction(FunctionModule *module,
                     const std::vector<std::string> &parameterNames);
  std::optional<Operand> emitValue(ExprNode *);
  std::optional<Operand> emitPlace(ExprNode *);
  Operand::SSA emitAlloc(const Operand &size, const Operand &alignment);
  Operand::SSA emitVecPtr(const Operand::SSA &self);
  Operand::SSA emitVecLength(const Operand::SSA &self);
  Operand::SSA emitVecCapacity(const Operand::SSA &self);
  void emitEqualFunction(const std::string &, const Type &);
  void registerEqualFunction(const std::string &, const Type &);
  bool generateIfExpr(IfExprNode *, BasicBlock *, const Operand &,
                      std::vector<std::pair<Operand, BasicBlock *>> *);
  TypeLayout layout(const TypeModule &type);

public:
  explicit IRGenerator(IRModules &modules) : modules(&modules) {}

  void generateBuiltinFunctions();

  void generateEqualFunctions();

  // Crate and attribute.
  std::any visitCrate(CrateNode *node) override;

  // Items.
  std::any visitFunctionDeclItem(FunctionDeclItemNode *node) override;

  std::any visitImplDeclItem(ImplDeclItemNode *node) override;

  // Statements.
  std::any visitEmptyStmt(EmptyStmtNode *node) override;

  std::any visitLetStmt(LetStmtNode *node) override;

  std::any visitExpressionStmt(ExpressionStmtNode *node) override;

  // Expressions without blocks.
  std::any visitUnitExpr(UnitExprNode *node) override;

  std::any visitPathInExpr(PathInExprNode *node) override;

  std::any visitLiteralExpr(LiteralExprNode *node) override;

  std::any visitUnaryExpr(UnaryExprNode *node) override;

  std::any visitBinaryExpr(BinaryExprNode *node) override;

  std::any visitComparisonExpr(ComparisonExprNode *node) override;

  std::any visitLogicExpr(LogicExprNode *node) override;

  std::any visitCastExpr(CastExprNode *node) override;

  std::any visitBorrowExpr(BorrowExprNode *node) override;

  std::any visitDereferenceExpr(DereferenceExprNode *node) override;

  std::any visitAssignmentExpr(AssignmentExprNode *node) override;

  std::any visitConstExpr(ConstExprNode *node) override;

  std::any visitArrayExpr(ArrayExprNode *node) override;

  std::any visitIndexExpr(IndexExprNode *node) override;

  std::any visitStructExpr(StructExprNode *node) override;

  std::any visitCallExpr(CallExprNode *node) override;

  std::any visitMethodCallExpr(MethodCallExprNode *node) override;

  std::any visitReturnExpr(ReturnExprNode *node) override;

  std::any visitAccessExpr(AccessExprNode *node) override;

  std::any visitContinueExpr(ContinueExprNode *node) override;

  std::any visitBreakExpr(BreakExprNode *node) override;

  // Expressions with blocks.
  std::any visitBlockExpr(BlockExprNode *node) override;

  std::any visitIfExpr(IfExprNode *node) override;

  std::any visitInfiniteLoopExpr(InfiniteLoopExprNode *node) override;

  std::any visitPredicateLoopExpr(PredicateLoopExprNode *node) override;
};
