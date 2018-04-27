// Copyright (c) 2018 QIHOO Inc
// Author: Chengyuxuan (chengyuxuan@360.cn)

#ifndef DAT_DATRIE_H
#define DAT_DATRIE_H
#include <iostream>
#include <vector>
using namespace std;
#include <string>
#include <unistd.h>
#include <assert.h>
#define INVALID  0x8000000000000000
/********核心思想*************/
/***Base[parent]+ch = child***/
/***check[child] = parent ****/
/*****************************/
class DATrie
{
public:
    DATrie()
        :base(1024,0), check(1024,INVALID), tail(256,0),_pos(1),size_(0)
    {
        base[0] = 1; // root 是下标 0 开始的，我们把它初始偏移量初始为1.这是因为如果从1开始那么可能在后面我们插一个二进制串0，第一个字符是0，base[1]+0 为1 ，因为初始的时候base[1]的base值初始化了，但是却没有check选择归主，这个时候插入应该是走
//冲突逻辑的但是。因为没有设置check却走了 一般逻辑会造成bug ，所以从0开始。
    }               // check 初始化为 INVALID 代表这些节点都没有父节点 ,选择INVALID是因为，最大的有符号long int 也比这个小，我们取pos的时候
                   // 都是 -pos , 即不可能取到这个值，那么就用这个值来充当 check中空闲空间的标志是极为合适的
                   //  pos 初始化为 1 是因为如果不初始化为1 初始化成 0 的话，那么第一个串插入时候 base 保存 -pos 的时候 还是0
                   //  遍历的时候就会跟base那些初始化的0混

    DATrie(const vector<int64_t> & _base , const vector<int64_t> & _check , const string & _tail,uint64_t pos )
      :base(_base),check(_check),tail(_tail),_pos(pos),size_(0)
    {}
    void ReSizeBaseCheck(int size )
    {
        base.resize(2*size,0);
        check.resize(2*size,INVALID);
    }
    /* void InSert( string str) */
    bool InSert( string  str)
    {
        assert(str.size()>0);
        size_t  ret = CharSize(str);
        if(ret > base.size())
         {
            ReSizeBaseCheck(base.size());
         }
        if(tail.size()-_pos < str.size())
           tail.resize(2*tail.size()); 
        str.push_back(0); // 因为string 中的结束符不是 \0 ， 所以这里加入一个\0用来区分 tail中的不同字符串
        string::iterator begin = str.begin();
        string::iterator end = str.end();
        volatile int64_t node = 0;
        int64_t child;
        unsigned char ch;
        int64_t pos;
        static uint64_t  NewOffset = 1; // 这里把offset 变成了静态的那么 多线程下是不安全的
        while (begin != end)
           {
             ch = *begin++;
             child = base[node] + ch;
             assert(child > 0);
             if(child >= base.size())
             {
                ReSizeBaseCheck(child);
             }
             if(check[child]== INVALID)
               {
                    check[child] = node;
                    break;
               }
             if(check[child] == node && check[child] >= 0 && base[child]<0)
              {
                  /*有冲突当前节点不再是单独节点了,单独节点表示从当前节点开始能够区分多个有共同前缀的字符串*/
                  int64_t i = 0;
                  pos = -base[child];
                  node = child;
                  while((ch=tail[pos++])!=0 && begin!=end)
                  {
                     if(ch == *begin)
                    {
                         for(NewOffset;check[NewOffset+ch/*base[i+ch]*/]!=INVALID;++NewOffset)
                           {
                              if(NewOffset+ch > check.size())
                                ReSizeBaseCheck(NewOffset+ch);
                           }                                               /*****冲突的四种情况**********/
                         base[node] = NewOffset;                           /*****************************/
                         check[NewOffset+ch] = node;                       /*  1. abcde / abcgh**********/
                         ++begin;                                          /*  2. abc  / abcde **********/
                         node = NewOffset+ch;                              /*  3. abcdef / abc **********/
                    }                                                      /*  4. abc0 / abc0 ***********/
                    else                                                   /*****************************/
                    {                   /*第四种情况是reducetrie中只有a节点，tail中有 "bc0" 现在插入"abc0"*/ 
                       break;
                    }
                  }
                  if(ch == 0 && *begin == 0)
                  {
                      for(NewOffset;check[NewOffset]!=INVALID;++NewOffset)
                      {
                          if(NewOffset > check.size())
                             ReSizeBaseCheck(NewOffset);
                      } 
                        base[node] = NewOffset;
                        check[NewOffset] = node;
                        base[NewOffset] = 0;
                     // cout << str.data()<<  pos << endl;
                      ++size_;
                      return true;// 情况4
                  }
                  else if(*begin==0) // begin == end 情况3 
                  {
                      for(NewOffset;check[NewOffset+ch]!=INVALID||check[NewOffset]!=INVALID;++NewOffset) 
                      {
                               cout << NewOffset << endl;
                               if(NewOffset+ch > check.size())
                                  ReSizeBaseCheck(NewOffset+ch);
                      }
                      check[NewOffset]=check[ch+NewOffset] = node;
                      base[node] = NewOffset;
                      base[ch+NewOffset] = -pos; // 没问题这里 对于情况3 当 d 与 0 逻辑到这里 ， pos 指向 e            
                      base[NewOffset+0] = 0;
                      ++size_;
                      return true;
                     // ch = 0;// 替换ch 这里 s1串的处理完了，交给下面公共代码去处理S2串
                  }
                  else
                  {  //情况1 与 2  
                     unsigned char S2Ch = *begin;
                    if(S2Ch+NewOffset > check.size())
                        ReSizeBaseCheck(S2Ch+NewOffset);
                     for(NewOffset;check[S2Ch+NewOffset]!=INVALID||check[ch+NewOffset]!=INVALID;++NewOffset)
                     {
                                 if(S2Ch+NewOffset > check.size())
                                   ReSizeBaseCheck(S2Ch+NewOffset);
                     }                                        
                                     // 这俩个for循环一个是处理第一个串的区分节点的对应的相应字符
                     base[node] = NewOffset; // 一个是处理第二个串的字符区分节点的相应字符
                     check[ch+NewOffset] = node;
                     check[S2Ch+NewOffset] = node;
                     if(ch == 0 )
                       base[ch+NewOffset] = 0;
                     else
                       base[ch+NewOffset] = -pos;
                     ch = S2Ch;//替换ch 这里 s1串的处理完了，交给下面公共代码去处理s2串
                     ++begin;// 这里存到tail数组的字符应该是 区分节点表示字符的下一个字符，   
                            // abcde  abcgh  这里S2串 区分节点代表的字符是 g 可存到tail中的应该是 h之后的字符 
                  }
                  break;    
              }
             if(check[child]!=node)
             {
               /*****位置冲突，此情况是2个父节点在竞争一个子节点*****/
               int64_t TroubleNode = check[child];
               vector<unsigned char>l1,l2;
               for(unsigned short i = 0; i < 256 ;++i)
               {
                 if(base[TroubleNode]+i > check.size())
                    ReSizeBaseCheck(base[TroubleNode]+i);
                 if(check[base[TroubleNode]+i]==TroubleNode) // base[base[TroubleNode]+i] 这里子节点的值无论大于0还是小于0我们都得到了
                   l1.push_back(i); // base[TroubleNode]+i 就是child啦 我们看child对应的 
                 if(check[base[node]+i]==node)//base是否有效即可, 有效就代表着那条边上的字符是被插入的字符
                   l2.push_back(i);
               }
               // 保证 base[root]不可被修改 TroubleNode == 0
               if(l1.size() > l2.size()||TroubleNode == 0 )
               {
                  l1.swap(l2);    //得到最少边的容器，并且确定到底修改那个节点
                  TroubleNode = node ;
               }
               auto size = l1.size();
               bool flag = true; // 这个flag值代表我们找到了适合的数字q ，使得所有子节点check[q+child] == 0 
               while(1) 
               {
                   uint64_t   aim = 0; 
                   for(decltype(size) j = 0; j < size;++j)
                  {
                     aim = NewOffset+l1[j];
                     if(aim > check.size())
                     {
                        ReSizeBaseCheck(aim); // 用来防止当寻找的位置超过check的长度的时候我们需要扩容
                     }
                     if(check[aim] != INVALID)
                     {
                        flag = false;
                        break;
                     }
                  }
                  if(flag)
                     break;  
                  ++NewOffset,flag = true; 
               }
               /*******现在把TroubleNode的所有子节点都更新到新的位置处*******/
               int64_t ChildTemp = 0;
               volatile int64_t NewChild = 0;
               for(auto & element : l1)
               {
                  /****** NewOffset  等于 base[TroubleNode]的新偏移量*******/
                  ChildTemp = base[TroubleNode]+element;
                  assert(ChildTemp < (int64_t)base.size()); // 这个不强转的话，就会无符号与有符号比较，太危险了，负数直接会溢出成为正数，所以必须把 size_t 转换为 int 
// 这里没有选择判断childtemp是否超过就扩容是因为，正确逻辑就孩子是不会超过base界限的，所以这里assert下
                  NewChild = element + NewOffset;
                  if(NewChild > base.size())
                       ReSizeBaseCheck(NewChild);
                  base[NewChild]=base[ChildTemp];//新位置接管每个子孩子旧位置的初始偏移量
                  check[NewChild] = TroubleNode;
                  if(base[ChildTemp]>0)
                  {
                      /****如果大于0说明该子节点也有自己孩子，那么我们需要让新的子节点接管老的子节点孩子****/
                      /****小于0那就说明没有子节点，直接把pos下标赋值给新节点即可就不用进这段逻辑了****/
                      int64_t grandson = 0;
                      for(unsigned short k = 0; k < 256 ; ++k)
                      {
                           if((grandson=base[ChildTemp]+k) >  check.size())
                                ReSizeBaseCheck(grandson); 
                           if(check[grandson]==ChildTemp)
                              {
                                    check[grandson] = NewChild;
                              }
                      }
                  }
                  /* 这个if是用来处理特殊情况，就是TroubleNode 和 node 是父子关系的时候，也就是ChildTemp 为node的时候需要更新node的新位置*/
                  if(ChildTemp == node)
                  {
                       node  = NewChild ;
                       cout << "NewChild :" << NewChild <<endl;
                       cout << "node     :" << node << endl;
                       cout << "ChildTemp :" << ChildTemp <<endl;
                  }
                  base[ChildTemp] = 0; // 设为无效
                  check[ChildTemp]= INVALID;
               }
               base[TroubleNode] = NewOffset; 
               --begin;
               continue; // 冲突解决重新再插入冲突字符
             }
             node = child; 
           }
           if(base[node] == 0)
              return false; // 走到这里了 插入了相同的串，导致最后一个node被赋值的时候，被赋值成0,这里直接返回就好
           /* 从 Tail 中把共同字符插入到reduce trie中完毕 ，现在处理tail 数组中的遗留问题*/
           /*情况3这种特殊的已经处理了 剩下的可以都当一种搞,而且原有的已经都处理完毕，只剩下待插入s2的处理*/
          pos = _pos;
          base[ch+base[node]] = -pos;

          while(begin != end)
          {
             tail[pos++] = *begin++;
          } 
          _pos = pos;
          ++size_;
          return true;
    }
    void OutPut(int fd)
    {
         assert(fd>0);
         size_t  size = base.size();
         write(fd,&size,sizeof(size_t));
         write(fd,&base[0],sizeof(int)*base.size());
         size = check.size();
         write(fd,&size,sizeof(size_t));
         write(fd,&check[0],sizeof(int)*check.size());
         size = tail.size();
         write(fd,&size,sizeof(size_t));
         write(fd,&tail[0],sizeof(unsigned char));
         write(fd,&_pos,sizeof(_pos));
    }
    /************************************************************************************************/
    /*** 当有 abc / abcdef插入的时候这俩个字符串都可以插入进去，并区分*******************************/
    /***是这样区分的，从 C节点开始，abc串的查找是 base[node+0] 与 base[node+'d'] 这里node为第节点c***/
    /***因为偏移量不同，所以查找的child就不同，故可以区分开******************************************/
    bool Find(string  str)
    {
        if(str.size()==0)
            return false;
        int64_t  node = 0;
        str.push_back(0);
        string::iterator begin = str.begin();
        string::iterator end = str.end();
        unsigned char ch = 0;
        //str.push_back(0); 写到这里会出错迭代器失效
        while(begin!=end)
        {
           ch = *begin;
           if(base[node=base[node]+ch]==0 && check[node] == INVALID)
             return false;
           else if(base[node] <= -1)
           {
               /***逻辑走到这里表明有一个相同字符的节点是区分节点，剩余的字符在tail中，需要去tail中比较****/
               /*****能做到这里说明  区分节点的字符 与 str 对应的字符 都是相同的，否则base[x] == 0 了***/
               /*****所以我们要 让 begin 向后走一位 才对，因为区分节点的字符已经比较过了*****/
               ++begin;
               int64_t pos = -base[node];
               while((ch=tail[pos++])&&begin!=end)
               {
                  if(ch!=*begin++)
                    return false;
               }
               if(++begin!=end)
                 return false;   //说明给的串 给 已有串长，已有串只是查找串的子串
               else 
                 return true;
               /* 上面++begin的原因是我们先前多push了个0进去.故我需要 ++begin使其先后走一步才正确*/
           }
          ++begin;
        }
        if(base[node]!=0)
           return false;
        else 
           return true;
        //因为我们把 \0插入到里面了，这里就是判断如果是\0结尾则正确
        //否则说明没插入过这个串。可能插入了abcde但是没插入abc
    }
 unsigned long Size()
 {
   return size_;
 }
void Fun()
{
  /*
  for(auto & i:base)
  {
     cout<< i <<" ";
  }
  cout << endl;
  */
  auto   size = check.size();
  decltype(size) i = 0;
  int64_t start = 0;
  for( i = 0; i < size; ++i)
   {
      if(check[i] > 0 && base[i]<0)
      {
          cout<<check[i] << " " << i <<" ";
          start = -base[i];
          cout << start << endl;
          while(tail[start]) cout<<tail[start++]<<" ";
          cout << endl;
      }
   } 
}
protected:
  size_t CharSize(string & str)
{
    size_t ret = 0;
    for(auto & i : str)
      ret +=i;
    return ret;
}

private:
    vector<int64_t> base;
    vector <int64_t> check;
    string tail;
    uint64_t  _pos; // tail  中下一个的位置
    uint64_t  size_;
};

#endif
