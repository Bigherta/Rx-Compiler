#pragma once
#include <cctype>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
struct position {
  std::size_t startLine;
  std::size_t startColumn;
  std::size_t endLine;
  std::size_t endColumn;
}; // Source positions use [start, end).
class ASTNode {
public:
  position pos;
  ASTNode(const position &pos) : pos(pos) {}
  ASTNode(position &&pos) : pos(std::move(pos)) {}
  virtual ~ASTNode() = default;
};

class TypeNode : public ASTNode {
public:
  using ASTNode::ASTNode;
  virtual ~TypeNode() = default;
};

class ItemNode : public ASTNode {
public:
  using ASTNode::ASTNode;
  virtual ~ItemNode() = default;
};

class ExprNode : public ASTNode {
public:
  using ASTNode::ASTNode;
  virtual ~ExprNode() = default;
};

class ExprWithBlockNode : public ExprNode {
public:
  using ExprNode::ExprNode;
  virtual ~ExprWithBlockNode() = default;
};

class ExprWithoutBlockNode : public ExprNode {
public:
  using ExprNode::ExprNode;
  virtual ~ExprWithoutBlockNode() = default;
};

class StmtNode : public ASTNode {
public:
  using ASTNode::ASTNode;
  virtual ~StmtNode() = default;
};

struct GenericArg {
  std::unique_ptr<TypeNode> genericArgType;
  GenericArg(std::unique_ptr<TypeNode> type)
      : genericArgType(std::move(type)) {}
};

struct PathSegment {
  std::string pathIdentSegment; // identifier / self / Self
  std::vector<GenericArg> genericArgs;
  PathSegment() = default;
  PathSegment(PathSegment &&) noexcept = default;
  PathSegment &operator=(PathSegment &&) noexcept = default;
};

struct PathNode {
  std::vector<PathSegment> segments;
};

enum class PrimitiveType {
  i32Type,
  isizeType,
  u32Type,
  usizeType,
  boolType,
  unitType,
};

class PrimitiveTypeNode : public TypeNode {
public:
  PrimitiveType primitiveType;
  PrimitiveTypeNode(const position &pos, PrimitiveType type)
      : TypeNode(pos), primitiveType(type) {}
};

enum class ContainerTypeKeyword {
  vec,
  box,
};

class PathInTypeNode : public TypeNode {
public:
  PathNode path;
  PathInTypeNode(const position &pos) : TypeNode(pos) {}
};

enum class ReferenceKind {
  AMP,
  ANDAND,
};
class ReferenceTypeNode : public TypeNode {
public:
  bool isMut;
  ReferenceKind refKind;
  std::unique_ptr<TypeNode> refType;
  ReferenceTypeNode(const position &pos, bool isMutable,
                    std::unique_ptr<TypeNode> type, ReferenceKind kind)
      : TypeNode(pos), isMut(isMutable), refType(std::move(type)),
        refKind(kind) {}
};

enum class Attribute {
  Copy,
  Clone,
  PartialEq,
  Eq,
};
class OuterAttributeNode : public ASTNode {
public:
  std::vector<Attribute> outerAttributes;
  OuterAttributeNode(const position &pos, std::vector<Attribute> &&attributes)
      : ASTNode(pos) {
    outerAttributes = std::move(attributes);
  }
};
class UnitExprNode : public ExprWithoutBlockNode {
public:
  using ExprWithoutBlockNode::ExprWithoutBlockNode;
};

class PathInExprNode : public ExprWithoutBlockNode {
public:
  using ExprWithoutBlockNode::ExprWithoutBlockNode;
  PathNode path;
};

class LiteralExprNode : public ExprWithoutBlockNode {
public:
  std::string literalValue;
  LiteralExprNode(const position &pos, const std::string &value)
      : ExprWithoutBlockNode(pos), literalValue(value) {}
};

enum class UnaryOperator {
  NEG,
  NOT,
};

class UnaryExprNode : public ExprWithoutBlockNode {
public:
  UnaryOperator op;
  std::unique_ptr<ExprNode> childExpr;
  UnaryExprNode(const position &pos, UnaryOperator op,
                std::unique_ptr<ExprNode> expr)
      : ExprWithoutBlockNode(pos), op(op), childExpr(std::move(expr)) {}
};

enum class BinaryOperator {
  Add,
  Sub,
  Mul,
  Div,
  Mod,

  BitAnd,
  BitOr,
  BitXor,

  ShiftLeft,
  ShiftRight
};

class BinaryExprNode : public ExprWithoutBlockNode {
public:
  BinaryOperator op;
  std::unique_ptr<ExprNode> leftChildExpr;
  std::unique_ptr<ExprNode> rightChildExpr;
  BinaryExprNode(const position &pos, BinaryOperator op,
                 std::unique_ptr<ExprNode> leftExpr,
                 std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), op(op), leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)) {}
};

enum class ComparisonOperator {
  equal,
  notEqual,
  greaterThan,
  lessThan,
  greaterThanOrEqual,
  lessThanOrEqual,
};

class ComparisonExprNode : public ExprWithoutBlockNode {
public:
  ComparisonOperator op;
  std::unique_ptr<ExprNode> leftChildExpr;
  std::unique_ptr<ExprNode> rightChildExpr;
  ComparisonExprNode(const position &pos, ComparisonOperator op,
                     std::unique_ptr<ExprNode> leftExpr,
                     std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), op(op), leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)) {}
};

enum class LogicOperator {
  LogicAnd,
  LogicOr,
};

class LogicExprNode : public ExprWithoutBlockNode {
public:
  LogicOperator op;
  std::unique_ptr<ExprNode> leftChildExpr;
  std::unique_ptr<ExprNode> rightChildExpr;
  LogicExprNode(const position &pos, LogicOperator op,
                std::unique_ptr<ExprNode> leftExpr,
                std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), op(op), leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)) {}
};

class CastExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<TypeNode> castType;
  std::unique_ptr<ExprNode> castValue;
  CastExprNode(const position &pos, std::unique_ptr<TypeNode> type,
               std::unique_ptr<ExprNode> value)
      : ExprWithoutBlockNode(pos), castType(std::move(type)),
        castValue(std::move(value)) {}
};

enum class ReferenceUnaryOperator {
  AMP,
  ANDAND,
};

class BorrowExprNode : public ExprWithoutBlockNode {
public:
  bool isMut;
  ReferenceUnaryOperator refOp;
  std::unique_ptr<ExprNode> borrowExpr;
  BorrowExprNode(const position &pos, bool isMut, ReferenceUnaryOperator op,
                 std::unique_ptr<ExprNode> expr)
      : ExprWithoutBlockNode(pos), isMut(isMut), refOp(op),
        borrowExpr(std::move(expr)) {}
};

class DereferenceExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> dereferenceExpr;
  DereferenceExprNode(const position &pos, std::unique_ptr<ExprNode> expr)
      : ExprWithoutBlockNode(pos), dereferenceExpr(std::move(expr)) {}
};

enum class AssignmentOperator {
  Assignment,
  additionAssignment,
  subtractionAssignment,
  multiplicationAssignment,
  divisionAssignment,
  moduloAssignment,
  ANDAssignment,
  ORAssignment,
  XORAssignment,
  leftShiftAssignment,
  rightShiftAssignment,
};
class AssignmentExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> leftChildExpr;
  std::unique_ptr<ExprNode> rightChildExpr;
  AssignmentOperator assignmentOp;
  AssignmentExprNode(const position &pos, AssignmentOperator op,
                     std::unique_ptr<ExprNode> leftExpr,
                     std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), assignmentOp(op),
        leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)) {}
};

class MagnitudeExprNode : public ExprWithoutBlockNode {
public:
  using ExprWithoutBlockNode::ExprWithoutBlockNode;
  std::variant<std::string, std::unique_ptr<PathInExprNode>> magnitude;
};

class ConstExprNode : public ExprWithoutBlockNode {
public:
  bool isNegative;
  std::variant<std::unique_ptr<LiteralExprNode>,
               std::unique_ptr<PathInExprNode>,
               std::unique_ptr<MagnitudeExprNode>>
      constValue;
  ConstExprNode(const position &pos, bool isNeg)
      : ExprWithoutBlockNode(pos), isNegative(isNeg) {}
};

class ArrayTypeNode : public TypeNode {
public:
  std::unique_ptr<TypeNode> typeParam;
  std::unique_ptr<ConstExprNode> length;
  ArrayTypeNode(const position &pos, std::unique_ptr<TypeNode> param,
                std::unique_ptr<ConstExprNode> len)
      : TypeNode(pos), typeParam(std::move(param)), length(std::move(len)) {}
};

class ArrayExprNode : public ExprWithoutBlockNode {
public:
  using ExprWithoutBlockNode::ExprWithoutBlockNode;
  std::variant<
      std::vector<std::unique_ptr<ExprNode>>,
      std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<ConstExprNode>>>
      arrayEntries;
};

class PostfixExprNode : public ExprWithoutBlockNode {
public:
  using ExprWithoutBlockNode::ExprWithoutBlockNode;
};

class IndexExprNode : public PostfixExprNode {
public:
  std::unique_ptr<ExprNode> arrayExpr;
  std::unique_ptr<ExprNode> indexExpr;
  IndexExprNode(const position &pos, std::unique_ptr<ExprNode> arrayExpr,
                std::unique_ptr<ExprNode> indexExpr)
      : PostfixExprNode(pos), arrayExpr(std::move(arrayExpr)),
        indexExpr(std::move(indexExpr)) {}
};

struct structExprField {
  std::string identifier;
  std::unique_ptr<ExprNode> fieldExpr;
  structExprField() = default;
  structExprField(structExprField &&) noexcept = default;
  structExprField &operator=(structExprField &&) noexcept = default;
};

class StructExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<PathInExprNode> path;
  std::vector<structExprField> structExprFields;
  StructExprNode(const position &pos, std::unique_ptr<PathInExprNode> path)
      : ExprWithoutBlockNode(pos), path(std::move(path)) {}
};

class CallExprNode : public PostfixExprNode {
public:
  std::unique_ptr<ExprNode> callee;
  std::vector<std::unique_ptr<ExprNode>> callParams;
  CallExprNode(const position &pos, std::unique_ptr<ExprNode> callee)
      : PostfixExprNode(pos), callee(std::move(callee)) {}
};

class MethodCallExprNode : public PostfixExprNode {
public:
  std::unique_ptr<ExprNode> callee;
  PathSegment callMethod;
  std::vector<std::unique_ptr<ExprNode>> callParams;
  MethodCallExprNode(const position &pos, std::unique_ptr<ExprNode> callee,
                     PathSegment &&seg)
      : PostfixExprNode(pos), callee(std::move(callee)),
        callMethod(std::move(seg)) {}
};

class ReturnExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> returnValue;
  ReturnExprNode(const position &pos)
      : ExprWithoutBlockNode(pos),
        returnValue(std::make_unique<UnitExprNode>(pos)) {}
  ReturnExprNode(const position &pos, std::unique_ptr<ExprNode> value)
      : ExprWithoutBlockNode(pos), returnValue(std::move(value)) {}
};

class AccessExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> accessObject;
  std::string identifier;
  AccessExprNode(const position &pos, std::unique_ptr<ExprNode> obj,
                 const std::string &name)
      : ExprWithoutBlockNode(pos), accessObject(std::move(obj)),
        identifier(name) {}
};

class ContinueExprNode : public ExprWithoutBlockNode {
  using ExprWithoutBlockNode::ExprWithoutBlockNode;
};

class BreakExprNode : public ExprWithoutBlockNode {
public:
  std::optional<std::unique_ptr<ExprNode>> loopValue;
  using ExprWithoutBlockNode::ExprWithoutBlockNode;
  BreakExprNode(const position &pos, std::unique_ptr<ExprNode> value)
      : ExprWithoutBlockNode(pos), loopValue(std::move(value)) {}
};

class EmptyStmtNode : public StmtNode {
  using StmtNode::StmtNode;
};

class LetStmtNode : public StmtNode {
public:
  bool isMut;
  std::string identifier;
  std::optional<std::unique_ptr<TypeNode>> type;
  std::unique_ptr<ExprNode> value;
  LetStmtNode(const position &pos, bool isMutable,
              std::unique_ptr<ExprNode> valueExpr)
      : StmtNode(pos), isMut(isMutable), value(std::move(valueExpr)) {}
};

class ExpressionStmtNode : public StmtNode {
public:
  using StmtNode::StmtNode;
  std::unique_ptr<ExprNode> expr;
  bool hasSemicolon;
};

class BlockExprNode : public ExprWithBlockNode {
public:
  using ExprWithBlockNode::ExprWithBlockNode;
  std::vector<std::unique_ptr<StmtNode>> statements;
  std::optional<std::unique_ptr<ExprNode>>
      tailExpr; // return value if there is a tail expression
};

class IfExprNode : public ExprWithBlockNode {
public:
  std::unique_ptr<ExprNode> conditions;
  std::unique_ptr<BlockExprNode> body;
  std::optional<std::unique_ptr<BlockExprNode>> elseExpr;
  std::optional<std::unique_ptr<IfExprNode>> elseIfExpr;
  IfExprNode(const position &pos, std::unique_ptr<ExprNode> cond,
             std::unique_ptr<BlockExprNode> body)
      : ExprWithBlockNode(pos), conditions(std::move(cond)),
        body(std::move(body)) {
    elseExpr = std::nullopt;
    elseIfExpr = std::nullopt;
  }
  IfExprNode(IfExprNode &&) noexcept = default;
  IfExprNode &operator=(IfExprNode &&) noexcept = default;
};

class InfiniteLoopExprNode : public ExprWithBlockNode {
public:
  using ExprWithBlockNode::ExprWithBlockNode;
  std::unique_ptr<BlockExprNode> body;
};

class PredicateLoopExprNode : public ExprWithBlockNode {
public:
  using ExprWithBlockNode::ExprWithBlockNode;
  std::unique_ptr<ExprNode> conditions;
  std::unique_ptr<BlockExprNode> body;
};

class LoopExprNode : public ExprWithBlockNode {
public:
  std::variant<std::unique_ptr<InfiniteLoopExprNode>,
               std::unique_ptr<PredicateLoopExprNode>>
      loopExpr;
  LoopExprNode(const position &pos,
               std::unique_ptr<InfiniteLoopExprNode> infinite,
               std::unique_ptr<PredicateLoopExprNode> predicate)
      : ExprWithBlockNode(pos) {
    if (infinite) {
      loopExpr = std::move(infinite);
    } else {
      loopExpr = std::move(predicate);
    }
  }
};

class CrateNode : public ASTNode {
public:
  std::vector<std::unique_ptr<ItemNode>> items;
  CrateNode() : ASTNode({0, 0, 0, 0}){};
};

struct SelfParam {
  bool isRef;
  bool isMut;
};
struct FunctionParam {
  bool isMut;
  std::string identifier;
  std::unique_ptr<TypeNode> paramType;
};
class FunctionDeclItemNode : public ItemNode {
public:
  std::string identifier;
  std::optional<std::unique_ptr<TypeNode>> returnType;
  std::optional<SelfParam> selfParam;
  std::vector<FunctionParam> functionParam;
  std::unique_ptr<BlockExprNode> functionBody;
  FunctionDeclItemNode(const position &pos, const std::string &name,
                       std::unique_ptr<BlockExprNode> expr)
      : ItemNode(pos), identifier(name), functionBody(std::move(expr)) {}
};

struct StructField {
  std::string identifier;
  std::unique_ptr<TypeNode> fieldType;
};

class StructDeclItemNode : public ItemNode {
public:
  std::vector<std::unique_ptr<OuterAttributeNode>> outerAttribute;
  std::string identifier;
  std::vector<StructField> structFields;
  StructDeclItemNode(const position &pos, const std::string &name)
      : ItemNode(pos), identifier(name) {}
};

class ConstantDeclItemNode : public ItemNode {
public:
  std::string identifier;
  std::unique_ptr<TypeNode> constType;
  std::unique_ptr<ConstExprNode> constValue;
  ConstantDeclItemNode(const position &pos, const std::string &name,
                       std::unique_ptr<TypeNode> type,
                       std::unique_ptr<ConstExprNode> value)
      : ItemNode(pos), identifier(name), constType(std::move(type)),
        constValue(std::move(value)) {}
};

class ImplDeclItemNode : public ItemNode {
public:
  using ItemNode::ItemNode;
  std::unique_ptr<TypeNode> implType;
  std::vector<std::variant<std::unique_ptr<ConstantDeclItemNode>,
                           std::unique_ptr<FunctionDeclItemNode>>>
      associatedItems;
};
