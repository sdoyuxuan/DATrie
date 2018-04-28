// Copyright (c) 2018  QIHOO INC
// Author : chengyuxuan (chengyuxuan@360.cn)
#ifndef Trie_benchmark_h 
#define Trie_benchmark_h

#include <string>
#include <vector>
#include <unistd.h>
#include <stdlib.h>
#include <random>
#include "util/env.h"
#include "benchmark.h"
#include "DATrieEngine.h"
#include "InitBlock.h"
#include "mystring.h"
using namespace std;
using namespace kvstore;


namespace kvstore {
  class KVTrieBenchMark : public BenchMark{
    public:
      KVTrieBenchMark(const BenchOptions * options , const vector<string> & filename , const char * who)
         :BenchMark(options),
         file_name_(filename),
         who_(who),
         keys_(),
         trie_(NULL),
         block_(NULL)
      {
         if(strcasecmp(who_,"Trie")==0 || strcasecmp(who_,"DATrie")==0) 
              trie_ = new DATrie();
         else 
        {     InitBlock::Init(file_name_);//在这里插入字符串
              block_ = new Block("./DmpKv");
        }
      }

      bool Init()
      {
            string object("0000616e1fe615a4232dcf50b837a0ec\tCggKBgir1Ds4DA==");
            std::ifstream  iofile;
            for(auto & file : file_name_)
            {
                  iofile.open(file);
				  if(!iofile)
				  {
					   LOG(ERROR) << "open file_name: " << file;
					   return false;
				  }
				  std::string line;
				  while(getline(iofile,line))
				  {
					 keys_.push_back(line);
				  }
                  iofile.close(); 
            }  
          if(strcasecmp(who_,"Trie")==0 || strcasecmp(who_,"DATrie")==0)
            goto DATrie;  
			assert(block_);
			block_->Init();
            return true;
      DATrie:
            bool debug = false;
            string key;
            string Value;
            size_t delimterpos = 0;
            for(auto & Kv : keys_)
            {
                delimterpos = Kv.find_first_of('\t');
                key.assign(Kv,0,delimterpos);
                Value.assign(Kv,delimterpos+1,Kv.size()-delimterpos-1);
                trie_->InSert(key,Value);
            }
            #ifdef DEBUG
            trie_->Fun();
            #endif
            return true;      
      }
      
      bool Destory()
      {
          delete block_;
          delete trie_;
          return true;
      }
      int rand_int ( int low ,int high)
      {
         random_device rdev;
         default_random_engine re(rdev());
         using Dist = uniform_int_distribution<int> ;
         Dist intd;
         return intd(re,Dist::param_type{low,high});
      }
      int64_t RunSingleBenchMark(ThreadArg * arg)
      {
         int64_t retlen = 0;
         uint64_t nums = 0;
         int random = rand_int(0,keys_.size()-1);
         string key = keys_[random];
         string value;
         if(strcasecmp(who_,"Trie")!=0 && strcasecmp(who_,"DATrie")!=0) 
         {
			key.assign(key,0,key.find_first_of('\t'));
			if(!block_->Get(Slice(key),&value).ok())
				  ++nums;// cout<<"not Find key"<<endl;
			retlen += key.size()+ value.size();
            if(nums > 0)
            {
                cout << nums << endl;
                cout << "Not Find Key :%"<<(nums*100)/keys_.size()<<endl;
            }
         }
         else 
         {
		   if(!trie_->Find(key,value))
		   {
				 ++nums;
		   }
		   retlen += key.size() + value.size();
           /* 
            if(nums > 0)
               cout << "Not Find Key :"<< nums <<endl;
           */
	     //多线程下 Find 没问题的，插入的话需要修改DATrie的代码得加锁
         }
          return  retlen;
      }
      ~KVTrieBenchMark()
      {
        Destory();
      }
      unsigned long Size()
      {
         cout <<"who:" <<who_ << endl;
         if(strcasecmp(who_,"DATrie")==0)
            return  trie_->Size();
         return keys_.size();
      }
    private:
     const char *  who_; 
     DATrie *  trie_;
     Block *  block_;
     vector<string> file_name_;
     vector<std::string> keys_;
  };
}
#endif
