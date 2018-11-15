#pragma once

#include <torch/csrc/jit/ir.h>
#include <torch/csrc/jit/alias_info.h>

namespace torch {
namespace jit {

/**
 * Alias analysis pass.
 *
 * This pass produces an AliasDb that contains aliasing and mutation
 * information about the graph. Callers (right now moveAfterTopologicallyValid)
 * can use this information to determine whether mutations to the graph are
 * safe, in that they don't reorder/change nodes in a way that affects output.
 *
 * Every value with a mutable type (Tensors, Lists, Tuples, etc.) will be
 * associated with one or more "alias sets". If two values share an alias set,
 * that means they may alias, implying that a mutation to one value cannot be
 * reordered past a use of the other. Only reordering two reads of an alias set
 * is considered safe.
 *
 * There is a special alias set called the "wildcard set", which indicates that
 * we're not sure what this value may alias. To be conservative, we consider
 * the wildcard alias set as potentially aliasing any value.
 */
class AliasDb {
 public:
  AliasDb(std::shared_ptr<Graph> graph) : graph_(graph) {
    analyze(graph_);
  }

  // Does `n` contain any wildcard aliases?
  bool hasWildcard(const Node* n) const;

  // Does `n` write to any alias sets?
  bool hasWrites(const Node* n) const;

  // Get all nodes that write to any alias set inputed/outputed by `n`
  std::unordered_set<Node*> getWritersForNode(const Node* n) const;

  // For debugging: print alias db state to stdout
  void dump() const;

 private:
  bool isMutableType(const Value* v);

  void analyze(std::shared_ptr<Graph> graph);
  void analyze(Block* block);
  void analyze(Node* node);

  void analyzeIf(Node* node);
  void analyzeLoop(Node* node);
  void analyzeSubgraph(Node* node);
  void analyzeCreator(Node* node);
  void analyzeExtractor(Node* node);
  void analyzeChunk(Node* node);

  Symbol getFreshAlias();
  void giveAlias(const Value* value, AliasInfo alias);
  void giveAlias(const Value* value, Symbol alias);

  std::shared_ptr<Graph> graph_;
  Symbol latestSymbol_ = Symbol::fromQualString("alias::0");
  std::unordered_map<const Value*, AliasInfo> valueToAlias_;
  std::unordered_map<Symbol, std::unordered_set<Node*>> aliasToWrites_;
};

inline TORCH_API AliasDb AliasAnalysis(std::shared_ptr<Graph> graph) {
  return AliasDb(graph);
}
} // namespace jit
} // namespace torch
