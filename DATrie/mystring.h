#ifndef MYSTRING_H 
#define MYSTRING_H
#include <iostream>
#include <string>
#include <mutex>
using namespace std;

template<class T1,class T2>
struct  Node
{
  public:
     Node()
       :Ch_(0),ValPos_(0)
     {

     }
     Node(T1 Ch , T2 ValPos)
       :Ch_(Ch),ValPos_(ValPos)
     {}
     Node(const Node<T1,T2> & object)
     {
             Ch_ = object.Ch_;
             ValPos_ = object.ValPos_;
     }
    Node & operator = (const Node<T1,T2> & object)
     {
             Ch_ = object.Ch_;
             ValPos_ = object.ValPos_;
             return *this;
     }
     bool  operator==(const Node<T1,T2> & object)
     {
         return Ch_==object.Ch_ && ValPos_ == object.ValPos_;
     }
     friend ostream& operator<<(ostream & _Cout,Node<T1,T2> & object)
     {
           _Cout<<object.Ch_<<" " << object.ValPos_;
           return _Cout;
     }
     T1 Ch_;
     T2  ValPos_;
};
typedef Node<unsigned char , uint32_t> TrieNode;
std::mutex mtx;
template<class T>
class MyString
{ 
  public:
  MyString(uint32_t nums = 16)
     :My_start_(new T[nums]),
      My_end_(My_start_+nums),
      Storage_(My_end_),
      Rep_(new int(1))
  {}

  MyString(const MyString<T> & object)
    :My_start_(object.My_start_),
     My_end_(object.My_end_),
     Storage_(object.Storage_),
     Rep_(object.Rep_)
  {
      mtx.lock();
      if(Rep_ == NULL )
      {
          cerr<< "对象已经释放" <<endl;
          exit(4);
      }
      else
      {
          ++(*Rep_);
      }
      mtx.unlock();
  }
  MyString<T> & operator=(const MyString<T> &  tmp)
  {
         mtx.lock();
         if(*tmp.Rep_ > 0 )
         {
            My_start_ = tmp.My_start_; 
            My_end_ = tmp.My_end_;
            Storage_ = tmp.Storage_;
            Rep_ = tmp.Rep_;
            ++(*Rep_);
         }
         else
         {
              cerr<< " 对象已经释放 " <<endl;
              exit(2);
         }
         mtx.unlock();
         return *this;
  }
  T& operator[](uint32_t pos)
  {
      if((*Rep_)>1)
      {
           mtx.lock();
           size_t storage = Storage_ - My_start_;
           size_t size = My_end_ - My_start_;
           T * TmpStart = new T[storage];
           Storage_ = TmpStart;
           while(My_start_ != My_end_)
           {
                *TmpStart++ = *My_start_++;
           }
           My_start_ = Storage_;
           My_end_ = My_start_ + size;
           Storage_ = My_start_ + storage;
           --(*Rep_);
           Rep_ = new int(1);
           mtx.unlock();
      }
      if(*Rep_ == 0 )
      {
          cerr<< " 已经释放" <<endl;
          exit(2);
      }
      return My_start_[pos];
  }
  ~MyString()
  {
    Destroy();
  }
  int32_t find_first_of( const T & ch)
  {
	  int32_t size = My_end_ - My_start_;
	  for(int i = 0 ; i < size; ++i)
	  {
		  if(My_start_[i] == ch)
			 return true;
	  }
	  return -1;
  }
  size_t size()
  {
       return My_end_ - My_start_;
  }
  size_t captical()
  {
       return Storage_ - My_start_;
  }
  void push_back(const T & Val)
  {
       mtx.lock();
	   T * TmpStart = My_start_  ;
	   size_t storage = Storage_ - TmpStart;
	   size_t size = My_end_ - My_start_;
	   My_start_ = new T[storage*2];
	   My_end_ = My_start_ + (size);
	   Storage_ = My_start_ + storage;
	   while(*My_start_++ = *My_end_++);
       *My_end_++ = Val;
	   --(*Rep_);
       Rep_ = new int(1);
       mtx.unlock();
  }
  virtual void Copy(const std::string & str)=0;
  void CopyForString(const string & str)
  {
          Copy(str);
  }
  protected:
  void Destroy()
  {
    mtx.lock();
    if(!(--(*Rep_)))
    {
         delete [] My_start_; 
         delete Rep_;
         Storage_=My_end_=My_start_= NULL;
         Rep_ = NULL;
    }
    mtx.unlock();
  }
  protected:
	  T * My_start_;
	  T * My_end_;
	  T * Storage_;
	  int * Rep_;
};
class mystring : public MyString<TrieNode>
{
  public:
       friend ostream & operator<<(ostream & _cout ,  mystring & object);
       mystring(uint32_t nums=16 ,unsigned char Ch = 0 )
         :MyString<TrieNode>(nums)
       {
             for(uint32_t i = 0 ; i < nums ; ++i)
             {
                   My_start_[i].Ch_ = Ch;
                   My_start_[i].ValPos_ = 0;
             }
       }
       void resize(size_t Size , const TrieNode  object=TrieNode())
       {
           if(Size < MyString<TrieNode>::captical())
           {
                  Size -= MyString<TrieNode>::size();
           }
           else 
           {
                  size_t oldsize = My_end_ - My_start_ ;
                  TrieNode * tmp = new TrieNode[Size<<1]; // 2*Size
                  memmove(tmp,My_start_,sizeof(TrieNode)*(oldsize)); 
                  Destroy();
                  My_start_ = tmp;
                  My_end_ = My_start_ + oldsize;
                  Storage_ = My_start_ + (size_t)(Size<<1);
                  Rep_ = new int(1);
                  Size -= oldsize;
           }
           while(Size--)
           {
                 *My_end_++ = object;
           }
       }
       ~mystring()
       {}
  private:
       void Copy(const std::string & str)
       {
            int size = str.size();
            MyString<TrieNode>::Destroy();
            My_start_  = new TrieNode[size];
            My_end_ = My_start_ + size;
            Storage_ = My_end_;
            Rep_ =  new int(1);
            for(int i = 0 ; i < size ; ++i)
            {
                 My_start_[i].Ch_ = str[i];
            }
       }
};
inline std::ostream & operator<< (std::ostream & _cout ,  mystring & object)
{
       TrieNode * TrieP = object.My_start_;  
       while(TrieP !=  object.My_end_)
       {
         _cout << TrieP -> Ch_;
         ++TrieP;
       }
       return _cout;
}

#endif
