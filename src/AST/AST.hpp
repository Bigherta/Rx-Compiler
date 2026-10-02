#include <cctype>
#include <cstddef>
#include <cstdint>
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
};
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
};

struct PathSegment {
  std::string pathIdentSegment; // identifier / self / Self
  std::vector<GenericArg> genericArgs;
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
  PrimitiveTypeNode(position pos, PrimitiveType type)
      : TypeNode(pos), primitiveType(type) {}
};

enum class ContainerTypeKeyword {
  vec,
  box,
};

class PathInTypeNode : public TypeNode {
public:
  PathNode path;
};

class ReferenceTypeNode : public TypeNode {
public:
  bool isMut;
  std::unique_ptr<TypeNode> refType;
  ReferenceTypeNode(position pos, bool isMutable,
                    std::unique_ptr<TypeNode> type)
      : TypeNode(pos), isMut(isMutable), refType(std::move(type)) {}
};

class OuterAttributeNode : public ASTNode {
public:
  uint8_t attribution = 0;
  // 1000 for Copy, 0100 for Clone, 0010 for PartialEq, 0001 for Eq
};
class UnitExprNode : public ExprWithoutBlockNode {
public:
  using ExprWithoutBlockNode::ExprWithoutBlockNode;
};

class PathInExprNode : public ExprWithoutBlockNode {
public:
  PathNode path;
};

class LiteralExprNode : public ExprWithoutBlockNode {
public:
  std::string literalValue;
  LiteralExprNode(position pos, const std::string &value)
      : ExprWithoutBlockNode(pos), literalValue(value) {}
};

enum class UnaryOperator {
  negative,
  negation,
};

class UnaryExprNode : public ExprWithoutBlockNode {
public:
  UnaryOperator op;
  std::unique_ptr<ExprNode> childExpr;
  UnaryExprNode(position pos, UnaryOperator op, std::unique_ptr<ExprNode> expr)
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
  BinaryExprNode(position pos, BinaryOperator op,
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
  ComparisonExprNode(position pos, ComparisonOperator op,
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
  LogicExprNode(position pos, LogicOperator op,
                std::unique_ptr<ExprNode> leftExpr,
                std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), op(op), leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)) {}
};

class CastExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<TypeNode> castType;
  std::unique_ptr<ExprNode> castValue;
  CastExprNode(position pos, std::unique_ptr<TypeNode> type,
               std::unique_ptr<ExprNode> value)
      : ExprWithoutBlockNode(pos), castType(std::move(type)),
        castValue(std::move(value)) {}
};

enum ReferenceUnaryOperator {
  SingleRef,
  DuplicateRef,
};

class BorrowExprNode : public ExprWithoutBlockNode {
public:
  bool isMut;
  ReferenceUnaryOperator refOp;
  std::unique_ptr<ExprNode> borrowExpr;
  BorrowExprNode(position pos, bool isMut, ReferenceUnaryOperator op,
                 std::unique_ptr<ExprNode> expr)
      : ExprWithoutBlockNode(pos), isMut(isMut), refOp(op),
        borrowExpr(std::move(expr)) {}
};

class DereferenceExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> dereferenceExpr;
  DereferenceExprNode(position pos, std::unique_ptr<ExprNode> expr)
      : ExprWithoutBlockNode(pos), dereferenceExpr(std::move(expr)) {}
};

class AssignmentExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> leftChildExpr;
  std::unique_ptr<ExprNode> rightChildExpr;
  AssignmentExprNode(position pos, std::unique_ptr<ExprNode> leftExpr,
                     std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)) {}
};

enum class CompoundAssignmentOperator {
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

class CompoundAssignmentExprNode : public ExprWithoutBlockNode {
public:
  CompoundAssignmentOperator op;
  std::unique_ptr<ExprNode> leftChildExpr;
  std::unique_ptr<ExprNode> rightChildExpr;
  CompoundAssignmentExprNode(position pos, CompoundAssignmentOperator op,
                             std::unique_ptr<ExprNode> leftExpr,
                             std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), op(op), leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)) {}
};

class MagnitudeExprNode : public ExprWithoutBlockNode {
public:
  std::variant<std::string, std::unique_ptr<PathInExprNode>> magnitude;
};

class ConstExprNode : public ExprWithoutBlockNode {
public:
  bool isNegative;
  std::variant<std::unique_ptr<LiteralExprNode>,
               std::unique_ptr<PathInExprNode>,
               std::unique_ptr<MagnitudeExprNode>>
      constValue;
};

class ArrayTypeNode : public TypeNode {
public:
  std::unique_ptr<TypeNode> param;
  std::unique_ptr<ConstExprNode> length;
};

class ArrayExprNode : public ExprWithoutBlockNode {
public:
  std::variant<
      std::vector<std::unique_ptr<ExprNode>>,
      std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<ConstExprNode>>>
      arrayEntries;
};

class IndexExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> arrayExpr;
  std::unique_ptr<ExprNode> indexExpr;
  IndexExprNode(position pos, std::unique_ptr<ExprNode> arrayExpr,
                std::unique_ptr<ExprNode> indexExpr)
      : ExprWithoutBlockNode(pos), arrayExpr(std::move(arrayExpr)),
        indexExpr(std::move(indexExpr)) {}
};

struct structExprField {
  std::string identifier;
  std::unique_ptr<ExprNode> fieldExpr;
};

class StructExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<PathInExprNode> path;
  std::vector<structExprField> structExprFields;
};

class CallExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> callee;
  std::vector<std::unique_ptr<ExprNode>> callParams;
};

class MethodCallExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> callee;
  PathSegment callMethod;
  std::vector<std::unique_ptr<ExprNode>> callParams;
};

class ReturnExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> returnValue;
};

class AccessExprNode : public ExprWithoutBlockNode {
public:
  std::unique_ptr<ExprNode> accessObject;
  std::string identifier;
};

class ContinueExprNode : public ExprWithoutBlockNode {
  using ExprWithoutBlockNode::ExprWithoutBlockNode;
};

class BreakExprNode : public ExprWithoutBlockNode {
public:
  std::optional<std::unique_ptr<ExprNode>> loopValue;
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
};

class ExpressionStmtNode : public StmtNode {
public:
  std::variant<std::unique_ptr<ExprWithBlockNode>,
               std::unique_ptr<ExprWithoutBlockNode>>
      expr;
  bool hasSemicolon;
};

class BlockExprNode : public ExprWithBlockNode {
public:
  std::vector<std::unique_ptr<StmtNode>> statements;
  std::optional<std::unique_ptr<ExprWithoutBlockNode>> tailExpr;
};

class IfExprNode : public ExprWithBlockNode {
public:
  std::unique_ptr<ExprNode> conditions;
  std::unique_ptr<BlockExprNode> body;
  std::optional<
      std::variant<std::unique_ptr<BlockExprNode>, std::unique_ptr<IfExprNode>>>
      elseExpr;
};

class InfiniteLoopExprNode : public ExprWithBlockNode {
public:
  std::unique_ptr<BlockExprNode> body;
};

class PredicateLoopExprNode : public ExprWithBlockNode {
public:
  std::unique_ptr<ExprNode> conditions;
  std::unique_ptr<BlockExprNode> body;
};

class LoopExprNode : public ExprWithBlockNode {
public:
  std::variant<std::unique_ptr<InfiniteLoopExprNode>,
               std::unique_ptr<PredicateLoopExprNode>>
      loopExpr;
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
};

struct structField {
  std::string identifier;
  std::unique_ptr<TypeNode> fieldType;
};

class StructDeclItemNode : public ItemNode {
public:
  OuterAttributeNode outerAttribute;
  std::string identifier;
  std::vector<structField> structFields;
};

class ConstantDeclItemNode : public ItemNode {
public:
  std::string identifier;
  std::unique_ptr<TypeNode> constType;
  std::unique_ptr<ConstExprNode> constValue;
};

struct AssociateItem {
public:
  std::variant<FunctionDeclItemNode, ConstantDeclItemNode> item;
};
class ImplDeclItemNode : public ItemNode {
public:
  std::unique_ptr<TypeNode> implType;
  std::vector<std::unique_ptr<AssociateItem>> associatedItems;
};
