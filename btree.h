#ifndef BTree_H
#define BTree_H
#include <iostream>
#include <vector>
#include <string>
#include "node.h"

using namespace std;

template <typename TK>
class BTree {
 private:
  Node<TK>* root;
  int M;  // grado u orden del arbol
  int n; // total de elementos en el arbol 

 public:
  BTree(int _M) : root(nullptr), M(_M), n(0) {}

  bool search(TK key);//indica si se encuentra o no un elemento
  void insert(TK key);//inserta un elemento
  void remove(TK key);//elimina un elemento
  int height();//altura del arbol. Considerar altura 0 para arbol vacio
  string toString(const string& sep);  // recorrido inorder
  vector<TK> rangeSearch(TK begin, TK end);

  TK minKey();  // minimo valor de la llave en el arbol
  TK maxKey();  // maximo valor de la llave en el arbol
  void clear(); // eliminar todos lo elementos del arbol
  int size(); // retorna el total de elementos insertados  
  
  // Construya un árbol B a partir de un vector de elementos ordenados
  static BTree* build_from_ordered_vector(vector<TK> elements, int _M);
  bool check_properties();

  ~BTree();

private:
  void insertNonFull(Node<TK>* node, TK key);
  void splitChild(Node<TK>* parent, int i);
  Node<TK>* searchNode(Node<TK>* node, TK key);
  int getHeight(Node<TK>* node);
  void inorder(Node<TK>* node, vector<TK>& result);
  TK getMin(Node<TK>* node);
  TK getMax(Node<TK>* node);
  void clearNode(Node<TK>* node);
  void removeFromNode(Node<TK>* node, TK key);
};

template <typename TK>
bool BTree<TK>::search(TK key) {
  return searchNode(root, key) != nullptr;
}

template <typename TK>
Node<TK>* BTree<TK>::searchNode(Node<TK>* node, TK key) {
  if (!node) return nullptr;
  int i = 0;
  while (i < node->count && key > node->keys[i]) i++;
  if (i < node->count && key == node->keys[i]) return node;
  if (node->leaf) return nullptr;
  return searchNode(node->children[i], key);
}

template <typename TK>
void BTree<TK>::insert(TK key) {
  if (!root) {
    root = new Node<TK>(M);
    root->keys[0] = key;
    root->count = 1;
    root->leaf = true;
    n = 1;
  } else {
    if (root->count == M - 1) {
      Node<TK>* newRoot = new Node<TK>(M);
      newRoot->leaf = false;
      newRoot->children[0] = root;
      splitChild(newRoot, 0);
      root = newRoot;
    }
    insertNonFull(root, key);
    n++;
  }
}

template <typename TK>
void BTree<TK>::splitChild(Node<TK>* parent, int i) {
  Node<TK>* fullChild = parent->children[i];
  Node<TK>* newChild = new Node<TK>(M);
  newChild->leaf = fullChild->leaf;
  int mid = (M - 1) / 2;
  newChild->count = M - 1 - mid - 1;
  for (int j = 0; j < newChild->count; j++) {
    newChild->keys[j] = fullChild->keys[j + mid + 1];
  }
  if (!fullChild->leaf) {
    for (int j = 0; j <= newChild->count; j++) {
      newChild->children[j] = fullChild->children[j + mid + 1];
    }
  }
  fullChild->count = mid;
  for (int j = parent->count; j > i; j--) {
    parent->children[j + 1] = parent->children[j];
  }
  parent->children[i + 1] = newChild;
  for (int j = parent->count - 1; j >= i; j--) {
    parent->keys[j + 1] = parent->keys[j];
  }
  parent->keys[i] = fullChild->keys[mid];
  parent->count++;
}

template <typename TK>
void BTree<TK>::insertNonFull(Node<TK>* node, TK key) {
  int i = node->count - 1;
  if (node->leaf) {
    while (i >= 0 && key < node->keys[i]) {
      node->keys[i + 1] = node->keys[i];
      i--;
    }
    node->keys[i + 1] = key;
    node->count++;
  } else {
    while (i >= 0 && key < node->keys[i]) i--;
    i++;
    if (node->children[i]->count == M - 1) {
      splitChild(node, i);
      if (key > node->keys[i]) i++;
    }
    insertNonFull(node->children[i], key);
  }
}

template <typename TK>
void BTree<TK>::remove(TK key) {
  if (!root) return;
  removeFromNode(root, key);
  if (root->count == 0) {
    Node<TK>* tmp = root;
    if (root->leaf) {
      root = nullptr;
    } else {
      root = root->children[0];
    }
    tmp->killSelf();
    delete tmp;
  }
  n--;
}

template <typename TK>
void BTree<TK>::removeFromNode(Node<TK>* node, TK key) {
  int idx = 0;
  while (idx < node->count && node->keys[idx] < key) idx++;
  
  if (idx < node->count && node->keys[idx] == key) {
    if (node->leaf) {
      for (int i = idx + 1; i < node->count; i++) {
        node->keys[i - 1] = node->keys[i];
      }
      node->count--;
    }
  } else {
    if (!node->leaf) {
      removeFromNode(node->children[idx], key);
    }
  }
}

template <typename TK>
int BTree<TK>::height() {
  if (!root) return 0;
  return getHeight(root);
}

template <typename TK>
int BTree<TK>::getHeight(Node<TK>* node) {
  if (!node) return 0;
  if (node->leaf) return 0;
  return 1 + getHeight(node->children[0]);
}

template <typename TK>
string BTree<TK>::toString(const string& sep) {
  vector<TK> result;
  inorder(root, result);
  string s = "";
  for (int i = 0; i < result.size(); i++) {
    if (i > 0) s += sep;
    s += to_string(result[i]);
  }
  return s;
}

template <typename TK>
void BTree<TK>::inorder(Node<TK>* node, vector<TK>& result) {
  if (!node) return;
  int i;
  for (i = 0; i < node->count; i++) {
    if (!node->leaf) {
      inorder(node->children[i], result);
    }
    result.push_back(node->keys[i]);
  }
  if (!node->leaf) {
    inorder(node->children[i], result);
  }
}

template <typename TK>
vector<TK> BTree<TK>::rangeSearch(TK begin, TK end) {
  vector<TK> all;
  inorder(root, all);
  vector<TK> result;
  for (auto key : all) {
    if (key >= begin && key <= end) {
      result.push_back(key);
    }
  }
  return result;
}

template <typename TK>
TK BTree<TK>::minKey() {
  vector<TK> all;
  inorder(root, all);
  if (all.empty()) return TK();
  return all[0];
}

template <typename TK>
TK BTree<TK>::getMin(Node<TK>* node) {
  if (!node) return TK();
  if (node->leaf) return node->keys[0];
  return getMin(node->children[0]);
}

template <typename TK>
TK BTree<TK>::maxKey() {
  vector<TK> all;
  inorder(root, all);
  if (all.empty()) return TK();
  return all[all.size() - 1];
}

template <typename TK>
TK BTree<TK>::getMax(Node<TK>* node) {
  if (!node) return TK();
  if (node->leaf) return node->keys[node->count - 1];
  return getMax(node->children[node->count]);
}

template <typename TK>
void BTree<TK>::clear() {
  clearNode(root);
  root = nullptr;
  n = 0;
}

template <typename TK>
void BTree<TK>::clearNode(Node<TK>* node) {
  if (!node) return;
  if (!node->leaf) {
    for (int i = 0; i <= node->count; i++) {
      clearNode(node->children[i]);
    }
  }
  node->killSelf();
  delete node;
}

template <typename TK>
int BTree<TK>::size() {
  return n;
}

template <typename TK>
BTree<TK>* BTree<TK>::build_from_ordered_vector(vector<TK> elements, int _M) {
  BTree<TK>* tree = new BTree<TK>(_M);
  for (auto elem : elements) {
    tree->insert(elem);
  }
  return tree;
}

template <typename TK>
bool BTree<TK>::check_properties() {
  if (!root) return true;
  vector<TK> keys;
  inorder(root, keys);
  for (int i = 1; i < keys.size(); i++) {
    if (keys[i] <= keys[i-1]) return false;
  }
  return true;
}

template <typename TK>
BTree<TK>::~BTree() {
  clearNode(root);
}

#endif
