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
  void removeFromLeaf(Node<TK>* node, int idx);
  void removeFromInternal(Node<TK>* node, int idx);
  void fixUnderflow(Node<TK>* node, int idx);
  void borrowFromLeft(Node<TK>* node, int idx);
  void borrowFromRight(Node<TK>* node, int idx);
  void mergeWithLeft(Node<TK>* node, int idx);
  void mergeWithRight(Node<TK>* node, int idx);
  int getMinKeys();
  bool checkNodeProperties(Node<TK>* node, bool isRoot, int& leafHeight, int currentHeight);
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
  
  int minKeys = getMinKeys();
  int mid = (M - 1) / 2;
  int keysToRight = M - 1 - mid - 1;
  
  // Para M impar (como 3), el hijo derecho puede quedar con 0 claves después del split estándar
  // Ajustar la distribución solo si podemos asegurar que ambos hijos tengan al menos minKeys
  if (keysToRight == 0 && mid >= minKeys + 1) {
    // Redistribuir: dar una clave al derecho (ajustar mid)
    keysToRight = 1;
    mid = mid - 1;  // Quitar una clave del izquierdo para darla al derecho
  }
  
  newChild->count = keysToRight;
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
    
    // Después de la inserción, verificar si algún hijo adyacente tiene menos de minKeys
    // Esto puede pasar cuando el split dejó un hijo con 0 claves y la inserción fue al otro hijo
    int minKeys = (node == root) ? 1 : getMinKeys();
    
    // Verificar el hijo donde se insertó y sus hermanos adyacentes
    int startIdx = (i-1 > 0) ? i-1 : 0;
    int endIdx = (i+1 < node->count) ? i+1 : node->count;
    for (int idx = startIdx; idx <= endIdx; idx++) {
      if (node->children[idx] && node->children[idx]->count < minKeys) {
        // Intentar pedir prestado del hermano izquierdo
        if (idx > 0 && node->children[idx-1] && node->children[idx-1]->count > minKeys) {
          borrowFromLeft(node, idx);
        }
        // O intentar pedir prestado del hermano derecho
        else if (idx < node->count && node->children[idx+1] && 
                 node->children[idx+1]->count > minKeys) {
          borrowFromRight(node, idx);
        }
      }
    }
  }
}

template <typename TK>
int BTree<TK>::getMinKeys() {
  return (M + 1) / 2 - 1;
}

template <typename TK>
void BTree<TK>::remove(TK key) {
  if (!root) return;
  
  Node<TK>* nodeWithKey = searchNode(root, key);
  if (!nodeWithKey) return;
  
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
      removeFromLeaf(node, idx);
    } else {
      removeFromInternal(node, idx);
    }
  } else {
    if (!node->leaf) {
      int minKeys = (node == root) ? 1 : getMinKeys();
      
      if (node->children[idx]->count < minKeys + 1) {
        fixUnderflow(node, idx);
        idx = 0;
        while (idx < node->count && node->keys[idx] < key) idx++;
        if (idx == node->count) {
          idx = node->count;
        }
      }
      
      removeFromNode(node->children[idx], key);
      
      if (idx >= 0 && idx <= node->count && node->children[idx]) {
        if (node->children[idx]->count < minKeys) {
          fixUnderflow(node, idx);
        }
      }
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
  for (size_t i = 0; i < result.size(); i++) {
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
void BTree<TK>::removeFromLeaf(Node<TK>* node, int idx) {
  for (int i = idx + 1; i < node->count; i++) {
    node->keys[i - 1] = node->keys[i];
  }
  node->count--;
}

template <typename TK>
void BTree<TK>::removeFromInternal(Node<TK>* node, int idx) {
  Node<TK>* leftChild = node->children[idx];
  Node<TK>* rightChild = node->children[idx + 1];
  int minKeys = (node == root) ? 1 : getMinKeys();
  
  if (leftChild->count > minKeys) {
    TK predecessor = getMax(leftChild);
    node->keys[idx] = predecessor;
    removeFromNode(leftChild, predecessor);
    if (leftChild->count < minKeys) {
      fixUnderflow(node, idx);
    }
  } else if (rightChild->count > minKeys) {
    TK successor = getMin(rightChild);
    node->keys[idx] = successor;
    removeFromNode(rightChild, successor);
    if (rightChild->count < minKeys) {
      fixUnderflow(node, idx + 1);
    }
  } else {
    TK predecessor = getMax(leftChild);
    node->keys[idx] = predecessor;
    removeFromNode(leftChild, predecessor);
    mergeWithRight(node, idx);
  }
}

template <typename TK>
void BTree<TK>::fixUnderflow(Node<TK>* node, int idx) {
  if (idx < 0 || idx > node->count) return;
  
  int minKeys = (node == root) ? 1 : getMinKeys();
  Node<TK>* child = node->children[idx];
  
  if (child->count >= minKeys) return;
  
  if (idx > 0 && node->children[idx - 1]->count > minKeys) {
    borrowFromLeft(node, idx);
    return;
  }
  
  if (idx < node->count && node->children[idx + 1] && node->children[idx + 1]->count > minKeys) {
    borrowFromRight(node, idx);
    return;
  }
  
  if (idx > 0) {
    mergeWithLeft(node, idx);
  } else if (idx < node->count) {
    mergeWithRight(node, idx);
  }
}

template <typename TK>
void BTree<TK>::borrowFromLeft(Node<TK>* node, int idx) {
  Node<TK>* child = node->children[idx];
  Node<TK>* leftSibling = node->children[idx - 1];
  
  if (!child->leaf) {
    for (int i = child->count; i >= 0; i--) {
      child->children[i + 1] = child->children[i];
    }
  }
  for (int i = child->count - 1; i >= 0; i--) {
    child->keys[i + 1] = child->keys[i];
  }
  
  child->keys[0] = node->keys[idx - 1];
  child->count++;
  
  if (!child->leaf) {
    child->children[0] = leftSibling->children[leftSibling->count];
  }
  
  node->keys[idx - 1] = leftSibling->keys[leftSibling->count - 1];
  leftSibling->count--;
}

template <typename TK>
void BTree<TK>::borrowFromRight(Node<TK>* node, int idx) {
  Node<TK>* child = node->children[idx];
  Node<TK>* rightSibling = node->children[idx + 1];
  
  child->keys[child->count] = node->keys[idx];
  child->count++;
  
  if (!child->leaf) {
    child->children[child->count] = rightSibling->children[0];
  }
  
  node->keys[idx] = rightSibling->keys[0];
  
  for (int i = 1; i < rightSibling->count; i++) {
    rightSibling->keys[i - 1] = rightSibling->keys[i];
  }
  
  if (!rightSibling->leaf) {
    for (int i = 1; i <= rightSibling->count; i++) {
      rightSibling->children[i - 1] = rightSibling->children[i];
    }
  }
  
  rightSibling->count--;
}

template <typename TK>
void BTree<TK>::mergeWithLeft(Node<TK>* node, int idx) {
  Node<TK>* child = node->children[idx];
  Node<TK>* leftSibling = node->children[idx - 1];
  
  leftSibling->keys[leftSibling->count] = node->keys[idx - 1];
  int oldLeftCount = leftSibling->count;
  leftSibling->count++;
  
  for (int i = 0; i < child->count; i++) {
    leftSibling->keys[leftSibling->count + i] = child->keys[i];
  }
  
  if (!child->leaf) {
    for (int i = 0; i <= child->count; i++) {
      leftSibling->children[oldLeftCount + 1 + i] = child->children[i];
    }
  }
  
  leftSibling->count += child->count;
  
  for (int i = idx - 1; i < node->count - 1; i++) {
    node->keys[i] = node->keys[i + 1];
  }
  
  for (int i = idx; i < node->count; i++) {
    node->children[i] = node->children[i + 1];
  }
  
  node->count--;
  
  child->killSelf();
  delete child;
}

template <typename TK>
void BTree<TK>::mergeWithRight(Node<TK>* node, int idx) {
  Node<TK>* child = node->children[idx];
  Node<TK>* rightSibling = node->children[idx + 1];
  
  child->keys[child->count] = node->keys[idx];
  int oldChildCount = child->count;
  child->count++;
  
  for (int i = 0; i < rightSibling->count; i++) {
    child->keys[child->count + i] = rightSibling->keys[i];
  }
  
  if (!child->leaf) {
    for (int i = 0; i <= rightSibling->count; i++) {
      child->children[oldChildCount + 1 + i] = rightSibling->children[i];
    }
  }
  
  child->count += rightSibling->count;
  
  for (int i = idx; i < node->count - 1; i++) {
    node->keys[i] = node->keys[i + 1];
  }
  
  for (int i = idx + 1; i < node->count; i++) {
    node->children[i] = node->children[i + 1];
  }
  
  node->count--;
  
  rightSibling->killSelf();
  delete rightSibling;
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
  for (size_t i = 1; i < keys.size(); i++) {
    if (keys[i] <= keys[i-1]) return false;
  }
  
  int leafHeight = -1;
  
  if (root->count < 1) return false;
  
  if (!root->leaf && (root->count + 1) < 2) return false;
  
  return checkNodeProperties(root, true, leafHeight, 0);
}

template <typename TK>
bool BTree<TK>::checkNodeProperties(Node<TK>* node, bool isRoot, int& leafHeight, int currentHeight) {
  if (!node) return true;
  
  int minKeys = getMinKeys();
  int minChildren = (M + 1) / 2;
  
  for (int i = 1; i < node->count; i++) {
    if (node->keys[i] <= node->keys[i-1]) return false;
  }
  
  if (isRoot) {
    if (node->count < 1) return false;
    if (node->count > M - 1) return false;
  } else {
    if (node->count < minKeys) return false;
    if (node->count > M - 1) return false;
  }
  
  if (node->leaf) {
    if (leafHeight == -1) {
      leafHeight = currentHeight;
    } else if (leafHeight != currentHeight) {
      return false;
    }
    return true;
  }
  
  int numChildren = node->count + 1;
  
  if (isRoot) {
    if (numChildren < 2) return false;
    if (numChildren > M) return false;
  } else {
    if (numChildren < minChildren) return false;
    if (numChildren > M) return false;
  }
  
  for (int i = 0; i <= node->count; i++) {
    if (!node->children[i]) return false;
  }
  
  for (int i = 0; i <= node->count; i++) {
    if (!checkNodeProperties(node->children[i], false, leafHeight, currentHeight + 1)) {
      return false;
    }
  }
  
  return true;
}

template <typename TK>
BTree<TK>::~BTree() {
  clearNode(root);
}

#endif
