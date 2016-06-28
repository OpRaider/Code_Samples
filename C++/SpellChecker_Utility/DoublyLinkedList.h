#ifndef DOUBLYLINKEDLIST_H
#define DOUBLYLINKEDLIST_H

#include <string>

using namespace std;


template <typename T>
class DoublyLinkedList
{
public:
    class Node
    {    
    public:        
        Node(T item){ numTimesFound = 0; data = item; next = 0; prev = 0; } 
        int numTimesFound;
        T data;
        Node* next;
        Node* prev;
    };

private:
    Node* head;
    Node* tail;
    int _size;
    
public:
    class iterator
    {
    public:
        Node* node;
        iterator(){ node = 0; }
        iterator(Node* n){ node = n; }
        iterator(int n){ node = 0; }
        iterator& operator=(iterator rhs){ node = rhs.node; }
        iterator& operator=(Node* n){ node = n; }
        iterator& operator=(int n){ node = 0; }
        iterator& operator++()
        {
            node = node->next;
            return *this;
        }
        iterator operator++(int)
        {
            iterator tmp(*this);
            operator++();
            return tmp;
        }
        iterator& operator--()
        {
            node = node->prev;
            return *this;
        }
        iterator operator--(int)
        {
            iterator tmp(*this);
            operator--();
            return tmp;
        }
        T& operator*(){ return node->data; }
        const T& operator*() const{ return node->data; }

    };
    friend bool operator!=(iterator& lhs, iterator& rhs){ return lhs.node != rhs.node; }
    friend bool operator==(iterator& lhs, iterator& rhs) { return !(lhs != rhs); }
    DoublyLinkedList()
    {
        head = 0;
        tail = 0;
        _size = 0;
    }

    ~DoublyLinkedList()
    {
        Node* current = head;
        while (current != 0)
        {
            Node* temp = current->next;
            delete current;
            current = temp;
        }
        head = 0;
        tail = 0;
    } 
    int size() const { return _size; }
    iterator begin() const { return iterator(head); }
    iterator end() const { return iterator(0); }

    T front() const { return head->data; }
    T back() const { return tail->data; }

    void push_front(const T data)
    {
        Node* newNode = new Node(data);
        if (isEmpty())
        {
            head = newNode;
            tail = newNode;
        }
        else
        {
            head->prev = newNode;
            newNode->next = head;
            head = newNode;
        }
        _size++;
    }

    void push_back(const T data)
    {
        Node* newNode = new Node(data);
        if (isEmpty())
        { 
            head = newNode;
            tail = newNode;
        }
        else 
        {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        _size++;
    }

    // insert calls push_back. That's all. Syntactic sugar.
    void insert(const T data)
    {
        push_back(data);
    }

    T& operator[](int index)
    {
        iterator it = begin();
        for (int i = 0; i < index; i++)
            it++;

        return *it;
    } 
    const T& operator[](int index) const
    {
        const iterator it = begin();
        for (int i = 0; i < index; i++)
            it++;
 
        return *it;
    }
    string& operator[](string firstWord)
    {
        DoublyLinkedList<pair<string, string> >::iterator it = findFirstWord(firstWord);
        return (*it).second;
    }

   
    void moveToFront(Node* n)
    {
        if(n == tail)
        {
            n->prev->next = 0;
            tail = n->prev;
            n->prev = 0;
            n->next = head;
            head->prev = n;
            head = n;
        }
        else if(n == head)
            ; // do nothing, already at front.
        else
        {
            n->next->prev = n->prev;
            n->prev->next = n->next;
            head->prev = n;
            n->next = head;
            n->prev = 0;
            head = n;
        }
    }
    
    // starts search at front, returns the node if found, otherwise returns null
    iterator find(string data)
    {
        iterator _null = 0; 
        if(isEmpty())
            return _null;
              
        iterator it = head;
        while (it != _null)
        {
            if (it.node->data == data)
            {
                it.node->numTimesFound += 1;
                
                if (it.node->numTimesFound >= 4)
                    moveToFront(it.node);
                    
                return it;
            }
            it++;
        }
        return _null;
    }
    iterator findPair(string data)
    {

        iterator _null = 0; 
        if(isEmpty())
            return _null;
              
        iterator it = head;
        while (it != _null)
        {
            if (it.node->data.first == data)
                return it;

            it++;
        }
        return _null;
    }
    bool isEmpty() const
    {
        return head == 0;
    }
    

private:
    iterator findFirstWord(string firstWord)
    {

        iterator _null = 0; 
        if(isEmpty())
            return _null;
              
        iterator it = head;
        while (it != _null)
        {
            if (it.node->data.first == firstWord)
                return it;

            it++;
        }
        return _null;
    }
};

#endif
