#pragma once
#include <any>
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

enum class nodeKind {
  // crate and attribute
  Crate,
  OuterAttribute,

  // type
  UnitType,
  PathInType,
  ReferenceType,
  ArrayType,

  // item
  FunctionDeclItem,
  StructDeclItem,
  ConstantDeclItem,
  ImplDeclItem,

  // statement
  EmptyStmt,
  LetStmt,
  ExpressionStmt,

  // expression without block
  UnitExpr,
  PathInExpr,
  LiteralExpr,
  UnaryExpr,
  BinaryExpr,
  ComparisonExpr,
  LogicExpr,
  CastExpr,
  BorrowExpr,
  DereferenceExpr,
  AssignmentExpr,
  MagnitudeExpr,
  ConstExpr,
  ArrayExpr,
  IndexExpr,
  StructExpr,
  CallExpr,
  MethodCallExpr,
  ReturnExpr,
  AccessExpr,
  ContinueExpr,
  BreakExpr,

  // expression with block
  BlockExpr,
  IfExpr,
  InfiniteLoopExpr,
  PredicateLoopExpr,
};

class ASTVisitor;

class ASTNode {
public:
  nodeKind kind;
  position pos;
  ASTNode(const position &pos) : pos(pos) {}
  ASTNode(position &&pos) : pos(std::move(pos)) {}
  virtual ~ASTNode() = default;
  virtual std::any accept(ASTVisitor *visitor) = 0;
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

struct GenericArgs {
  std::vector<GenericArg> genericArgs;
  bool hasLifeTime = false;
};

struct PathSegment {
  std::string pathIdentSegment; // identifier / self / Self
  GenericArgs args;
  PathSegment() = default;
  PathSegment(PathSegment &&) noexcept = default;
  PathSegment &operator=(PathSegment &&) noexcept = default;
};

struct PathNode {
  std::vector<PathSegment> segments;
};

class UnitTypeNode : public TypeNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  UnitTypeNode(const position &pos) : TypeNode(pos) {
    kind = nodeKind::UnitType;
  }
};

enum class ContainerTypeKeyword {
  vec,
  box,
};

class PathInTypeNode : public TypeNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  PathNode path;
  PathInTypeNode(const position &pos) : TypeNode(pos) {
    kind = nodeKind::PathInType;
  }
};

enum class ReferenceKind {
  AMP,
  ANDAND,
};
class ReferenceTypeNode : public TypeNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  bool isMut;
  ReferenceKind refKind;
  std::unique_ptr<TypeNode> refType;
  ReferenceTypeNode(const position &pos, bool isMutable,
                    std::unique_ptr<TypeNode> type, ReferenceKind kind)
      : TypeNode(pos), isMut(isMutable), refKind(kind),
        refType(std::move(type)) {
    this->kind = nodeKind::ReferenceType;
  }
};

enum class Attribute {
  Copy,
  Clone,
  PartialEq,
  Eq,
};
class OuterAttributeNode : public ASTNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::vector<Attribute> outerAttributes;
  OuterAttributeNode(const position &pos, std::vector<Attribute> &&attributes)
      : ASTNode(pos) {
    outerAttributes = std::move(attributes);
    kind = nodeKind::OuterAttribute;
  }
};
class UnitExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  UnitExprNode(const position &pos) : ExprWithoutBlockNode(pos) {
    kind = nodeKind::UnitExpr;
  }
};

class PathInExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  PathNode path;
  PathInExprNode(const position &pos) : ExprWithoutBlockNode(pos) {
    kind = nodeKind::PathInExpr;
  }
};

class LiteralExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::string literalValue;
  LiteralExprNode(const position &pos, const std::string &value)
      : ExprWithoutBlockNode(pos), literalValue(value) {
    kind = nodeKind::LiteralExpr;
  }
};

enum class UnaryOperator {
  NEG,
  NOT,
};

class UnaryExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  UnaryOperator op;
  std::unique_ptr<ExprNode> childExpr;
  UnaryExprNode(const position &pos, UnaryOperator op,
                std::unique_ptr<ExprNode> expr)
      : ExprWithoutBlockNode(pos), op(op), childExpr(std::move(expr)) {
    kind = nodeKind::UnaryExpr;
  }
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
  virtual std::any accept(ASTVisitor *visitor) override;
  BinaryOperator op;
  std::unique_ptr<ExprNode> leftChildExpr;
  std::unique_ptr<ExprNode> rightChildExpr;
  BinaryExprNode(const position &pos, BinaryOperator op,
                 std::unique_ptr<ExprNode> leftExpr,
                 std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), op(op), leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)) {
    kind = nodeKind::BinaryExpr;
  }
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
  virtual std::any accept(ASTVisitor *visitor) override;
  ComparisonOperator op;
  std::unique_ptr<ExprNode> leftChildExpr;
  std::unique_ptr<ExprNode> rightChildExpr;
  ComparisonExprNode(const position &pos, ComparisonOperator op,
                     std::unique_ptr<ExprNode> leftExpr,
                     std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), op(op), leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)) {
    kind = nodeKind::ComparisonExpr;
  }
};

enum class LogicOperator {
  LogicAnd,
  LogicOr,
};

class LogicExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  LogicOperator op;
  std::unique_ptr<ExprNode> leftChildExpr;
  std::unique_ptr<ExprNode> rightChildExpr;
  LogicExprNode(const position &pos, LogicOperator op,
                std::unique_ptr<ExprNode> leftExpr,
                std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), op(op), leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)) {
    kind = nodeKind::LogicExpr;
  }
};

class CastExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<TypeNode> castType;
  std::unique_ptr<ExprNode> castValue;
  CastExprNode(const position &pos, std::unique_ptr<TypeNode> type,
               std::unique_ptr<ExprNode> value)
      : ExprWithoutBlockNode(pos), castType(std::move(type)),
        castValue(std::move(value)) {
    kind = nodeKind::CastExpr;
  }
};

enum class ReferenceUnaryOperator {
  AMP,
  ANDAND,
};

class BorrowExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  bool isMut;
  ReferenceUnaryOperator refOp;
  std::unique_ptr<ExprNode> borrowExpr;
  BorrowExprNode(const position &pos, bool isMut, ReferenceUnaryOperator op,
                 std::unique_ptr<ExprNode> expr)
      : ExprWithoutBlockNode(pos), isMut(isMut), refOp(op),
        borrowExpr(std::move(expr)) {
    kind = nodeKind::BorrowExpr;
  }
};

class DereferenceExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<ExprNode> dereferenceExpr;
  DereferenceExprNode(const position &pos, std::unique_ptr<ExprNode> expr)
      : ExprWithoutBlockNode(pos), dereferenceExpr(std::move(expr)) {
    kind = nodeKind::DereferenceExpr;
  }
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
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<ExprNode> leftChildExpr;
  std::unique_ptr<ExprNode> rightChildExpr;
  AssignmentOperator assignmentOp;
  AssignmentExprNode(const position &pos, AssignmentOperator op,
                     std::unique_ptr<ExprNode> leftExpr,
                     std::unique_ptr<ExprNode> rightExpr)
      : ExprWithoutBlockNode(pos), leftChildExpr(std::move(leftExpr)),
        rightChildExpr(std::move(rightExpr)), assignmentOp(op) {
    kind = nodeKind::AssignmentExpr;
  }
};

class MagnitudeExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::variant<std::string, std::unique_ptr<PathInExprNode>> magnitude;
  MagnitudeExprNode(const position &pos) : ExprWithoutBlockNode(pos) {
    kind = nodeKind::MagnitudeExpr;
  }
};

class ConstExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  bool isNegative;
  std::variant<std::unique_ptr<LiteralExprNode>,
               std::unique_ptr<PathInExprNode>,
               std::unique_ptr<MagnitudeExprNode>>
      constValue;
  ConstExprNode(const position &pos, bool isNeg)
      : ExprWithoutBlockNode(pos), isNegative(isNeg) {
    kind = nodeKind::ConstExpr;
  }
};

class ArrayTypeNode : public TypeNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<TypeNode> typeParam;
  std::unique_ptr<ConstExprNode> length;
  ArrayTypeNode(const position &pos, std::unique_ptr<TypeNode> param,
                std::unique_ptr<ConstExprNode> len)
      : TypeNode(pos), typeParam(std::move(param)), length(std::move(len)) {
    kind = nodeKind::ArrayType;
  }
};

class ArrayExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::variant<
      std::vector<std::unique_ptr<ExprNode>>,
      std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<ConstExprNode>>>
      arrayEntries;
  ArrayExprNode(const position &pos) : ExprWithoutBlockNode(pos) {
    kind = nodeKind::ArrayExpr;
  }
};

class PostfixExprNode : public ExprWithoutBlockNode {
public:
  using ExprWithoutBlockNode::ExprWithoutBlockNode;
};

class IndexExprNode : public PostfixExprNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<ExprNode> arrayExpr;
  std::unique_ptr<ExprNode> indexExpr;
  IndexExprNode(const position &pos, std::unique_ptr<ExprNode> arrayExpr,
                std::unique_ptr<ExprNode> indexExpr)
      : PostfixExprNode(pos), arrayExpr(std::move(arrayExpr)),
        indexExpr(std::move(indexExpr)) {
    kind = nodeKind::IndexExpr;
  }
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
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<PathInExprNode> path;
  std::vector<structExprField> structExprFields;
  StructExprNode(const position &pos, std::unique_ptr<PathInExprNode> path)
      : ExprWithoutBlockNode(pos), path(std::move(path)) {
    kind = nodeKind::StructExpr;
  }
};

class CallExprNode : public PostfixExprNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<ExprNode> callee;
  std::vector<std::unique_ptr<ExprNode>> callParams;
  CallExprNode(const position &pos, std::unique_ptr<ExprNode> callee)
      : PostfixExprNode(pos), callee(std::move(callee)) {
    kind = nodeKind::CallExpr;
  }
};

class MethodCallExprNode : public PostfixExprNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<ExprNode> callee;
  PathSegment callMethod;
  std::vector<std::unique_ptr<ExprNode>> callParams;
  MethodCallExprNode(const position &pos, std::unique_ptr<ExprNode> callee,
                     PathSegment &&seg)
      : PostfixExprNode(pos), callee(std::move(callee)),
        callMethod(std::move(seg)) {
    kind = nodeKind::MethodCallExpr;
  }
};

class ReturnExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::optional<std::unique_ptr<ExprNode>> returnValue;
  ReturnExprNode(const position &pos)
      : ExprWithoutBlockNode(pos), returnValue(std::nullopt) {
    kind = nodeKind::ReturnExpr;
  }
  ReturnExprNode(const position &pos, std::unique_ptr<ExprNode> value)
      : ExprWithoutBlockNode(pos), returnValue(std::move(value)) {
    kind = nodeKind::ReturnExpr;
  }
};

class AccessExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<ExprNode> accessObject;
  std::string identifier;
  AccessExprNode(const position &pos, std::unique_ptr<ExprNode> obj,
                 const std::string &name)
      : ExprWithoutBlockNode(pos), accessObject(std::move(obj)),
        identifier(name) {
    kind = nodeKind::AccessExpr;
  }
};

class ContinueExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  ContinueExprNode(const position &pos) : ExprWithoutBlockNode(pos) {
    kind = nodeKind::ContinueExpr;
  }
};

class BreakExprNode : public ExprWithoutBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::optional<std::unique_ptr<ExprNode>> loopValue;
  BreakExprNode(const position &pos)
      : ExprWithoutBlockNode(pos), loopValue(std::nullopt) {
    kind = nodeKind::BreakExpr;
  }
  BreakExprNode(const position &pos, std::unique_ptr<ExprNode> value)
      : ExprWithoutBlockNode(pos), loopValue(std::move(value)) {
    kind = nodeKind::BreakExpr;
  }
};

class EmptyStmtNode : public StmtNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  EmptyStmtNode(const position &pos) : StmtNode(pos) {
    kind = nodeKind::EmptyStmt;
  }
};

class LetStmtNode : public StmtNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  bool isMut;
  std::string identifier;
  std::optional<std::unique_ptr<TypeNode>> type;
  std::unique_ptr<ExprNode> value;
  LetStmtNode(const position &pos, bool isMutable,
              std::unique_ptr<ExprNode> valueExpr)
      : StmtNode(pos), isMut(isMutable), value(std::move(valueExpr)) {
    kind = nodeKind::LetStmt;
  }
};

class ExpressionStmtNode : public StmtNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<ExprNode> expr;
  bool hasSemicolon;
  ExpressionStmtNode(const position &pos) : StmtNode(pos) {
    kind = nodeKind::ExpressionStmt;
  }
};

class BlockExprNode : public ExprWithBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::vector<std::unique_ptr<StmtNode>> statements;
  std::optional<std::unique_ptr<ExprNode>>
      tailExpr; // return value if there is a tail expression
  BlockExprNode(const position &pos) : ExprWithBlockNode(pos) {
    kind = nodeKind::BlockExpr;
  }
};

class IfExprNode : public ExprWithBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
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
    kind = nodeKind::IfExpr;
  }
  IfExprNode(IfExprNode &&) noexcept = default;
  IfExprNode &operator=(IfExprNode &&) noexcept = default;
};

class InfiniteLoopExprNode : public ExprWithBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<BlockExprNode> body;
  InfiniteLoopExprNode(const position &pos) : ExprWithBlockNode(pos) {
    kind = nodeKind::InfiniteLoopExpr;
  }
};

class PredicateLoopExprNode : public ExprWithBlockNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<ExprNode> conditions;
  std::unique_ptr<BlockExprNode> body;
  PredicateLoopExprNode(const position &pos) : ExprWithBlockNode(pos) {
    kind = nodeKind::PredicateLoopExpr;
  }
};

class CrateNode : public ASTNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::vector<std::unique_ptr<ItemNode>> items;
  CrateNode(const position &pos) : ASTNode(pos) { kind = nodeKind::Crate; }
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
  virtual std::any accept(ASTVisitor *visitor) override;
  std::string identifier;
  GenericArgs args;
  std::optional<std::unique_ptr<TypeNode>> returnType;
  std::optional<SelfParam> selfParam;
  std::vector<FunctionParam> functionParam;
  std::unique_ptr<BlockExprNode> functionBody;
  FunctionDeclItemNode(const position &pos, const std::string &name,
                       std::unique_ptr<BlockExprNode> expr)
      : ItemNode(pos), identifier(name), functionBody(std::move(expr)) {
    kind = nodeKind::FunctionDeclItem;
  }
};

struct StructField {
  std::string identifier;
  std::unique_ptr<TypeNode> fieldType;
};

class StructDeclItemNode : public ItemNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::vector<std::unique_ptr<OuterAttributeNode>> outerAttribute;
  std::string identifier;
  std::vector<StructField> structFields;
  StructDeclItemNode(const position &pos, const std::string &name)
      : ItemNode(pos), identifier(name) {
    kind = nodeKind::StructDeclItem;
  }
};

class ConstantDeclItemNode : public ItemNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::string identifier;
  std::unique_ptr<TypeNode> constType;
  std::unique_ptr<ConstExprNode> constValue;
  ConstantDeclItemNode(const position &pos, const std::string &name,
                       std::unique_ptr<TypeNode> type,
                       std::unique_ptr<ConstExprNode> value)
      : ItemNode(pos), identifier(name), constType(std::move(type)),
        constValue(std::move(value)) {
    kind = nodeKind::ConstantDeclItem;
  }
};

class ImplDeclItemNode : public ItemNode {
public:
  virtual std::any accept(ASTVisitor *visitor) override;
  std::unique_ptr<TypeNode> implType;
  std::vector<std::variant<std::unique_ptr<ConstantDeclItemNode>,
                           std::unique_ptr<FunctionDeclItemNode>>>
      associatedItems;
  ImplDeclItemNode(const position &pos) : ItemNode(pos) {
    kind = nodeKind::ImplDeclItem;
  }
};
