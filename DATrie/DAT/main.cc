#include"../DATrie.h"
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>
#define SIZE 4096*4096
void usage()
{
   cout<< "Please Enter ./xxx Create DataPath Or ./xxx Load DataPath"<<endl;
}
int GetFile(const char * path)
{
     umask(0); 
     int fd = 0;
     do
     {
         fd = open(path,O_RDWR|O_APPEND);
         if(fd == -1 && errno == ENOENT)
           fd = open(path,O_CREAT|O_RDWR,0644);
         if(fd == -1 && errno != EINTR)
            perror("open"),exit(3);
     }while(fd < 0);
     return fd;
}
void ReadData(int fd , string & str)
{
      char * buff  = new char[SIZE];
      int sz = 0;
      while(sz = read(fd,buff,SIZE-1))
      {
           /*不过在Linux 2.6 中如果是低速IO 网络和磁盘 I/O 被信号中断都会自动重启可以看APUE信号部分介绍*/
           if(sz == -1 && errno!= EINTR)
             perror("read"),exit(4);
           buff[sz] = 0;
           str.append(buff);
      }    
      delete [] buff;
}

void CreateData(const char * FilePath)
{
       DATrie object;
       int outfd = GetFile("DATKV");
       int filefd = GetFile(FilePath);
       string buff;
       string temp;
       ReadData(filefd,buff);
       unsigned long  endpos = 0; // 结束位置
       unsigned long  delipos = 0;//delimiter 分隔符
       unsigned long  startpos = 0; // 初始位置
       size_t strsize = buff.size();
       char bar[102]={0};
       int barpos = 0;
       const char * p = "|/-\\";
       printf("进度:\n");
       while(startpos < strsize && (endpos=buff.find_first_of('\n',startpos))!=string::npos)
       {
           if((delipos = buff.find_first_of('\t',startpos))!= string::npos)
           {
                 temp.assign(buff.data()+startpos,buff.data()+delipos);
                 object.InSert(temp);
           }
           else
           {
                 cout<< "找不到该分隔符" <<endl;
                 exit(5);
           }
           bar[barpos]='#';
           bar[barpos+1] = 0;
           printf("[%-100s]%d%c\r",bar,barpos,p[barpos&3]);
           barpos = (startpos*100)/strsize;
           /******只存了key没有存value*****/
           startpos = endpos+1;
           buff[delipos] = 0;
           buff[endpos] = 0;
       }
       barpos = 100;
       printf("[%-100s]%d%c\r\n",bar,barpos,p[barpos&3]);
       object.OutPut(outfd);
}
void LoadData(const char * FilePath)
{
      vector<int> base,check;
      string tail;
      unsigned long pos = 0;
      auto size = base.size();
      int fd = GetFile(FilePath);
      read(fd,&size,sizeof(size));
      base.resize(size);
      read(fd,&base[0],size*sizeof(int));
      read(fd,&size,sizeof(size));
      check.resize(size);
      read(fd,&check[0],size*sizeof(int));
      read(fd,&size,sizeof(size));
      tail.resize(size);
      read(fd,&tail[0],size*sizeof(unsigned char));
      read(fd,&pos,sizeof(pos));
      DATrie object(base,check,tail,pos);
      string temp;
      while(1)
      {
          cout << "请输入要查找的字符串#";
          cin >> temp;
          if(object.Find(temp))
            cout << "该字符串存在" <<endl;
          else
            cout << "该字符串不存在" <<endl;
          cout << "If you want to  exit ? yes or no #"<<endl;
          cin >> temp;
          if(strcasecmp(temp.data(),"yes")==0)
             break;
      }
 }     
      
      
int main(int argc , char * argv[])
{
    if(argc != 3)
      usage(),exit(1);
    string option(argv[1]);
    if(strcasecmp(option.data(),"create")==0)
      CreateData(argv[2]);
    else if(strcasecmp(option.data(),"Load")==0)
      LoadData(argv[2]);
    else
    {
      cout<<"输入有误"<<endl;
      exit(2);
    }
}
