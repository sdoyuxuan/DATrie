#include <iterator>
#include "block.h"
#include "block_builder.h"
#include <boost/shared_ptr.hpp>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <glog/logging.h>
#ifndef ksnappy 
#define ksnappy 1 
#define SIZE 4096
#define KVSIZE 4096
typedef boost::shared_ptr<kvstore::db::BlockBuilder> SpBlockBuilder;
typedef boost::shared_ptr<const kvstore::BuilderOptions> Spoption;
using namespace kvstore;
using namespace kvstore::db;
using namespace std;
void usage()
{
  std::cout<<"Please Select Create or Load"<<std::endl;
  std::cout<<"Load: ./a.out Load FilePath"<<std::endl;
  std::cout<<"create:./a.out Create FilePath"<<std::endl;
}
int  GetFile(const char * FilePath);
BlockBuilder::EntriesIterator * StartUp()
{
    kvstore::BuilderOptions * Poptions = new kvstore::BuilderOptions;
    Poptions->version = 1;
    Poptions->compress = ksnappy;
    Poptions->segment_size_threshold=4096;
    Poptions->element_size_limit=4096;
    kvstore::db::BlockBuilder *  Pbuilder= new kvstore::db::BlockBuilder(Poptions);
    Pbuilder->Init(); 
    BlockBuilder::EntriesIterator *  ret = new BlockBuilder::EntriesIterator(Poptions,Pbuilder);
    return ret;
}
void ReadData (int fd , std::string & buff)
{
    char * ReadBuff = new char[SIZE];
    int sz = 0;
    do
    {
       sz=read(fd,ReadBuff,SIZE);
       if(sz == -1 && errno!= EINTR)
         perror("read"),exit(3);
       buff.append(ReadBuff,sz);
    }while(sz==SIZE);
    delete [] ReadBuff;
}
void LoadData(BlockBuilder::EntriesIterator * PEntriesIter,const char *FilePath)
{
    int fd = GetFile(FilePath);
    int outfd = GetFile("./DmpKv");
    std::string buff;
    std::string & Rrep = PEntriesIter->block_builder_->rep_;
    ReadData(fd,buff);
   // char keybuff[KVSIZE] = {0};
    PEntriesIter->block_builder_->BuildHeader();
    write(outfd,&Rrep[0],Rrep.size());//这里BuildHeader后调用 ADD，每次ADD都会清空rep_缓冲区所以得每次都得先写到文件中
    size_t pos = 0;
    size_t startpos = 0;
    size_t  endpos = 0; 
    kvstore::Slice Tmp;
    size_t strsize = buff.size();
    while(startpos < strsize && ((pos = buff.find_first_of(startpos,'\n'))!=std::string::npos))
    {
         
         if((endpos = buff.find_first_of(startpos,'\t'))!=std::string::npos)
         {
            assert(startpos<endpos&&endpos<pos);
            Tmp=PEntriesIter->block_builder_->Add(kvstore::Slice(&buff[0]+startpos,endpos-startpos),kvstore::Slice(&buff[0]+endpos+1,pos-endpos-1));
            write(outfd,Tmp.data(),Tmp.size()); 
         } 
         else
         {
           cout<<"找不到空格，数据格式错误"<<endl;
           exit(5);
         }
         buff[pos]=0;
         buff[endpos]=0;
         startpos=pos+1;     
    }
    PEntriesIter->block_builder_->FinishAdd();
    /*data写入已完毕剩下的是index(Entries)写入*/
    /*Index(Entries) 写入开始*/
    PEntriesIter->block_builder_->StartEntries();
    while(true)
    {
      PEntriesIter->Next();
      if(PEntriesIter->Valid())
      {
          Tmp = PEntriesIter->Value();
          write(outfd,Tmp.data(),Tmp.size());
      }
      else
      {
          break;
      }
    }
    /*剩下就是Bucket 与 meta的写入*/
    Tmp = PEntriesIter->block_builder_->BuildBucket();
    write(outfd,Tmp.data(),Tmp.size()); 
    Tmp=PEntriesIter->block_builder_->BuildMeta();
    write(outfd,Tmp.data(),Tmp.size()); 
    close(fd);
    close(outfd); 
}
int  GetFile(const char * FilePath)
{
   umask(0); 
   int ret = 0;
     do
    {
       ret=open(FilePath,O_RDWR|O_APPEND);
       if(ret == -1 && errno ==ENOENT)
        ret = open(FilePath,O_RDWR|O_CREAT,0644);
       if(ret == -1 && errno != EINTR)
         perror("open"),exit(2);
    }while(ret < 0);
    return ret;
}
void GetKey(Block & block)
{
    std::string buff;// 不考虑性能的情况下使用 cin 考虑使用scanf
    std::string value;
    while(1)
    {
      cout<<"Please Enter key or exit#"<<endl;
      cin >> buff;
      if(buff.compare("exit")==0)
         break;
      if((block.Get(Slice(buff),&value)).ok())
      {
         cout << value << endl; 
      }
      else
      {
         //LOG(INFO) << "GetKey :" << <<endl;
         cout << "not found:"<<buff<<endl;
      }
    }
}
void printmeta(const Meta & _meta)
{
    cout << _meta.ToString() << endl;
}
void Reponse(Block & block )
{
     std::string buff;
     char options=0;
     while(true)
     {
       cout <<"g:查询key"<<endl;
       cout <<"p:打印所有key"<<endl;
       cout <<"m:打印meta信息"<<endl;
       cout << "Pleaes Enter : ";
       std::cin >> options;
        switch(options)
	   {
		 case 'g':
		  GetKey(block);
		  goto end;
         case 'p':
          block.printKeys();
          break;
         case 'm':
          printmeta(block.meta());
          break;
		 default:
		  cout<<"目前没有功能"<<endl;
		  break;
	   }
     }
     end:
      return ;
}       
int main(int argc , char * argv[])
{ 
    if(argc != 3)
      usage(),exit(1);
    FLAGS_stderrthreshold = 2;
    google::InitGoogleLogging(argv[0]);
    if(strcasecmp(argv[1],"Create")==0)
    {
      BlockBuilder::EntriesIterator * PEntriesIter = StartUp();
       SpBlockBuilder SpBuilder(PEntriesIter->block_builder_);//因为迭代器
       //的析构函数中什么都没有做。那么我们必须的自己负责释放动态内存，
      //所以这里我们得自己显示的析构这些内存 
       Spoption SpOp(PEntriesIter->options_);
       LoadData(PEntriesIter,argv[2]);
    }
    else if(strcasecmp(argv[1],"Load")==0)
    {
         Block block(argv[2]);
         block.Init();
         Reponse(block);
    }
    else
    {
      usage();
      exit(6);
    }
    return 0;
}
#endif
