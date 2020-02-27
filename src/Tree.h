#ifndef VECTOR_TREE_H_
#define VECTOR_TREE_H_

#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <vector>

#include "llvm/Support/raw_ostream.h"

using namespace std;

//
// class TreeNode
//
//
class TreeNode {
 private:
  std::string data_;
  // std::vector<TreeNode *> child_;
  bool discovered_;

 public:
  // friend class Tree;

  TreeNode(string data) : data_{data}, discovered_{false} {};

  TreeNode(const TreeNode &from) {
    data_ = from.data_;
    discovered_ = from.discovered_;
  }
  ~TreeNode() {}

  std::string getData() const { return data_; }

  bool isDiscovered() const { return discovered_; }
  void setDiscovered() { discovered_ = true; }
  void resetDiscovered() { discovered_ = false; }

  void dump() { llvm::outs() << "[" << data_ << "] "; }

  virtual void visit() { llvm::outs() << "( " << data_ << ") "; }
};

//////////////////////
// class Tree
//
//
class Tree {
 public:
  typedef TreeNode *TreeNodePtr;
  typedef std::vector<TreeNodePtr> VectorTreePtr;

 private:
  // Adjacency list.
  // This is a reasonable structure since types are
  // typically going to be of small depth.
  std::map<TreeNodePtr, std::vector<TreeNodePtr> > adj_list_;
  TreeNodePtr root_;

 public:
  void dump() {
    for (auto const &entry : adj_list_) {
      auto node{entry.first};
      auto edges{entry.second};
      cout << node->getData() << " => size: " << edges.size() << "\n";
      for (auto const &edge_node : edges) {
        cout <<  "   " << edge_node->getData();
      }
      cout << "\n";
    }
  }

  std::size_t size() const { return adj_list_.size(); }

  void setRoot(const TreeNodePtr from) { root_ = from; }
  const TreeNodePtr getRoot() const { return root_; }

  TreeNodePtr addNode(std::string data) {
    TreeNodePtr new_node{new TreeNode(data)};
    VectorTreePtr empty_edges{};
    adj_list_.emplace(std::make_pair(new_node, empty_edges));
    return new_node;
  }

  void addEdge(const TreeNodePtr from, const TreeNodePtr to) {
    auto source{adj_list_.find(from)};

    if (source == adj_list_.end()) {
      return;
    }

    auto &edges{source->second};
    // Insert it into the beginning of the vector.
    //edges.insert(edges.begin(), to);
    edges.push_back(to);
    //push_back(to);
  }

  void resetDiscovered() {
    for (auto const &node : adj_list_) {
      node.first->resetDiscovered();
    }
  }

  void dft(TreeNodePtr root) {
    resetDiscovered();

    std::stack<TreeNodePtr> visit{};
    visit.push(root);
    //cout << "dft: " << root->getData() << "\n";

    while (!visit.empty()) {
      // node is a TreeNodePtr
      auto &node{visit.top()};
      //cout << "dft: callback function " << node->getData() << "\n";
      node->visit();
      // Call back function.
      visit.pop();

      if (!node->isDiscovered()) {
        node->setDiscovered();

        auto source{adj_list_.find(node)};

       //   cout << "dft: finding " << node << "  " << node->getData() <<  " \n";
        if (source == adj_list_.end()) {
        //  cout << "dft: " << node << " not found\n";
          return;
        }

        //cout << "dft: add all the edges\n";
        auto edges{source->second};
        for (auto &node : edges) {
         // cout << "dft: => include an edge\n";
          visit.push(node);
        }
      }
    }
  }
};

#endif
