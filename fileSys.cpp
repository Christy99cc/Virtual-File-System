#include<cstdio>
#include<iostream>
#include<fstream>
#include<string>
#include<cstring>
#include <windows.h>
#include<conio.h>
#include<ctime>
#include<cstdlib>

#define BLOCKSIZE 1024							    //盘块大小为1024 = 1KB
#define BLOCKNUM 128								//盘块数量
#define DISKSIZE BLOCKSIZE*BLOCKNUM					//磁盘大小 1024*128
#define DIRSIZE BLOCKSIZE/sizeof(FileControlBlock)  //目录文件数量

using namespace std;

/*————————————枚举类型定义——————————————*/
//文件属性
enum FileAttrib {
	ReadOnly = 1,     //只读
	WriteOnly = 2,    //只写
	ReadAndWrite = 3  //读写
};

//文件类型
enum FileType {
	Neither,
	OFileType = 1, //文件类型
	DirType = 2    //目录类型
};

/*————————————结构体定义——————————————*/
//时间
typedef struct DateTime {
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
}DateTime;

struct FileControlBlock //文件控制块 FCB
{
	char selfName[20];               //文件名称
	FileAttrib selfAttrib;           //文件属性
	FileType selfType;               //文件类型
	DateTime createTime;             //创建时间
	int selfSize;                    //文件大小
	int upperBlockId;                //上一级的盘块号
	int selfBlockId;                 //自身的盘块号

	void initial();
	void initTime();
	void printTime();
	int getSize();
};

//初始化文件控制块
void FileControlBlock::initial()
{
	selfName[0] = '\0';
	selfAttrib = ReadAndWrite;						//默认可读可写
	selfType = Neither;
	selfSize = upperBlockId = selfBlockId = 0;
}

//初始化文件控制块的创建时间——以当前时间作为创建时间
void FileControlBlock::initTime()
{
	time_t timep;
	time(&timep);
	struct tm *p = localtime(&timep);
	createTime.year = 1900 + p->tm_year;
	createTime.month = p->tm_mon + 1;
	createTime.day = p->tm_mday;
	createTime.hour = p->tm_hour;
	createTime.minute = p->tm_min;
	createTime.second = p->tm_sec;
}

//打印文件的创建时间
void FileControlBlock::printTime(){
	cout << createTime.year << "/" << createTime.month << "/" << createTime.day << " ";
	cout << createTime.hour << ":" << createTime.minute << ":" << createTime.second <<endl;
}

//目录
struct Ddirectory
{
	FileControlBlock subFCB[DIRSIZE];
	int subDirNum;                          //子目录的数量
	int subFileNum;                         //子文件的数量
	int subNum;                             //子文件和子目录的总数量

	//对于当前目录来说，它的名字，上一级目录的块号，自身所在的块号
	void initial(char* selfName, int upperBlockId, int selfBlockId);
	int getSubDirNum();
	int getSubFileNum();                         //子文件的数量
	int getSubNum();
};

//初始化一个新目录
void Ddirectory::initial(char* selfName, int upperBlockId, int selfBlockId)
{
	//先将本目录格式化，0号位置留下存储本目录的信息
	for (int i = 1; i < DIRSIZE; i++)
	{
		subFCB[i].selfType = Neither;             //该目录下无子文件或子目录
		subFCB[i].upperBlockId = selfBlockId;     //该目录的id号sub的上一级的id号
	}

	//设置该目录自身信息，储存在0号位
	subFCB[0].selfSize = 0;
	subFCB[0].selfBlockId = selfBlockId;
	subFCB[0].upperBlockId = upperBlockId;
	strcpy(subFCB[0].selfName, selfName);
	subFCB[0].initTime();

	//该目录下无子文件或子目录
	subDirNum = subFileNum = subNum = 0;
}

//得到当前目录下目录文件的数量
int Ddirectory::getSubDirNum() {
	int cnt = 0;
	for (int i = 1; i < DIRSIZE; i++)
		if (subFCB[i].selfType == DirType)	cnt++;
	return cnt;
}

//得到当前目录下普通文件的数量
int Ddirectory::getSubFileNum() {
	int cnt = 0;
	for (int i = 1; i < DIRSIZE; i++)
		if (subFCB[i].selfType == OFileType)	cnt++;
	return cnt;
}

//得到当前目录下文件的总数
int Ddirectory::getSubNum() {
	return getSubDirNum() + getSubFileNum();
}

//Disk结构
struct Disk
{
	Ddirectory root;                      //根目录
	int FAT[BLOCKNUM], FAT2[BLOCKNUM];    // FAT
	char data[BLOCKNUM - 3][BLOCKSIZE];   //数据块内容
	void initial();
};

//初始化磁盘
void Disk::initial()
{
	memset(FAT, 0, BLOCKNUM);
	memset(FAT, 0, BLOCKNUM);
	memset(data, 0, sizeof(data));
	FAT[0] = FAT[1] = -2;
	root.initial((char*)"root", 2, 2);
}

/*————————————全局变量的定义——————————————*/
FILE *filePtr;                                   //文件指针 VFS
string currentPath = "root/";                    //当前路径
int currentBlockId = 2;                          //当前目录所在的盘块号
const char* VirtualDisk = "VirtualDisk.bin";     //磁盘名  !!!必须加const，为了方便使用fopen函数，得使用char*
string inpCmd;                                   //输入的command
Disk *diskPtr;                                   //虚拟磁盘的对象
char* startAddress;                              //虚拟磁盘首地址      

//得到普通文件的大小
int FileControlBlock::getSize() {
	int i = 0;
	for (; i < BLOCKSIZE; i++)
	{
		if (diskPtr->data[selfBlockId - 3][i] == 4)
			break;
	}
	return i;
}   

/*—————除结构体函数以外其他函数的统一声明，方便查找—————*/
/*颜色*/ 
void color(short x);//自定义函根据参数改变颜色
//保存至磁盘
bool saveToDisk();
//从磁盘加载至内存
bool loadFromDisk();
//格式化的欢迎界面
void welcomeInterface();
//初始化VFS
void initialVFS();
//返回当前目录的指针
/*---------------------------其他函数的声明---------------------*/ 
Ddirectory* getCurrentDir();
//当前目录下重名目录文件的检查  若有重名文件返回下标
//寻找当前目录下是否存在subDirName名字的目录文件
int findSameSubDirName(Ddirectory* curDir, char* subDirName);
//查当前目录下的重名文件，重名返回下标，否则返回0
int findSameSubFileName(Ddirectory* curDir, char* subFileName);
//查找空闲文件控制块，返回i，如果已经满了就返回-1
int findFreeFcb(Ddirectory* curDir);
//查空白FAT，返回fat的下标，如果已经满了就返回-1
int findFreeFat(); 
//根据原来的路径，分析得到它的目录指针，所在的盘块号，以及绝对路径
bool parsePath(char* orinPath, Ddirectory* parseToDir, int& toCurBlockId, string& toAbsPath); 
//根据绝对路径分析出最后的文件名
char* parseFileName(char* orinPath);

/*-------------功能函数的声明------------*/ 
//mkdir 为当前目录创建子目录，名称需要符合规范
bool mkDir(char* subDirName);
//rmdir 删除当前目录下的文件夹
bool rmDir(char * delSubDirName);
//touch 创建文件
//说明：为了方便复制的操作，将载入和同步写在了主函数里
bool touch(char* subFileName, Ddirectory* curDir, int& tempSubFileFcbId, int& tempFatId);
//echo 写文件内容
bool echo(char* subFileName);
//more 查看文件内容
bool more(char* subFileName);
//del 删除普通文件
bool del(char* delsubFileName);
//dir list
bool dir();
//cd 改变工作路径，进入下级目录或者返回上级目录
//CHECK：针对每一个线程改变工作路径
bool cd(char *name);
bool copy(char *subFileName, char *toPath);//还需要完善
//退出
bool exitSys();
//help，打印帮助信息
void helpPrint();

/*——————————控制台颜色设置————————————*/
void color(short x) //自定义函根据参数改变颜色   
{
	if (x >= 0 && x <= 15)//参数在0-15的范围颜色  
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), x);    //只有一个参数，改变字体颜色   
	else//默认的颜色白色  
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

/*—————————虚拟磁盘的读写—————————————*/
//保存至磁盘
bool saveToDisk()
{
	filePtr = fopen(VirtualDisk, "wb+");
	if (filePtr != NULL) //wb+读写打开二进制文件
	{
		fwrite(startAddress, sizeof(char), DISKSIZE, filePtr);
		fclose(filePtr);
		return true;
	}
	else return false;
}

//从磁盘加载至内存
bool loadFromDisk()
{
	filePtr = fopen(VirtualDisk, "rb");
	if (filePtr != NULL)  //rb二进制只读形式打开文件
	{
		fread(startAddress, sizeof(char), DISKSIZE, filePtr);
		return true;
	}
	else return false;
}

//格式化的欢迎界面
void welcomeInterface()
{
	cout << "********************************************************" << endl;
	cout << "******                                            ******" << endl;
	cout << "******------------欢迎来到虚拟文件系统------------******" << endl;
	cout << "******                                            ******" << endl;
	cout << "********************************************************" << endl;
}

//初始化VFS
void initialVFS()
{
	cout << endl;
	cout << "需要格式化虚拟文件系统，所有数据将被清除。（";
	color(4);		//红色
	cout << "警告";
	color(16);		//恢复默认颜色
	cout << "）\n请选择[y/Y]继续，或者[n/N]退出。\n请输入：";
	while (true)
	{
		char type = getchar();
		if (type == 'y' || type == 'Y')						//如果选择y/N就进行初始化
		{
			cout << "格式化虚拟文件系统中..." << endl;
			currentBlockId = 2; //root
			currentPath = "root/";
			diskPtr->initial();								//磁盘恢复初始状态
			saveToDisk();									//初始好后保存至磁盘
			//进行初始化后退出while输入的循环
			cout << "格式化完成。" << endl;
			break;
		}
		else if (type == 'n' || type == 'N')
			exit(0);
		else
			cout << "格式错误，需使用[y/Y]或者[n/N]进行选择，请输入:";
	}
}

//返回BlockId目录的指针，默认返回当前目录的指针
Ddirectory* getCurrentDir()
{
	Ddirectory *curDir;											//返回当前目录的指针
	if (currentBlockId == 2)
		curDir = &(diskPtr->root);
	else
		curDir = (Ddirectory*)(diskPtr->data[currentBlockId - 3]);
	return curDir;
}

//重载上面的函数 
Ddirectory* getCurrentDir(int BlockId)
{
	Ddirectory *curDir;											//返回blockID目录的指针
	if (BlockId == 2)
		curDir = &(diskPtr->root);
	else
		curDir = (Ddirectory*)(diskPtr->data[BlockId - 3]);
	return curDir;
}

//当前目录下重名目录文件的检查  若有重名文件返回下标，否则返回0
//寻找当前目录下是否存在subDirName名字的目录文件
int findSameSubDirName(Ddirectory* curDir, char* subDirName)
{
	for (int i = 1; i < DIRSIZE; i++)
		if ((curDir->subFCB[i].selfType == DirType) && (strcmp(subDirName, curDir->subFCB[i].selfName) == 0))
			//CHECK:判断信息一开始写错了。。。一个是DirType，一个是用strcmp
			return i;
	return 0;
}

//查当前目录下的重名文件，重名返回下标，否则返回0
int findSameSubFileName(Ddirectory* curDir, char* subFileName)
{
	for (int i = 1; i < DIRSIZE; i++)
		if ((curDir->subFCB[i].selfType == OFileType) && (strcmp(subFileName, curDir->subFCB[i].selfName) == 0))
			return i;
	return 0;
}

//查找空闲文件控制块，返回i，如果已经满了就返回-1
int findFreeFcb(Ddirectory* curDir)
{
	int i = 1;
	for (i = 1; i < DIRSIZE; i++)
		if (curDir->subFCB[i].selfType == Neither) 			//既不是目录文件也不是文本文件的为空
			return i;
	if (i == DIRSIZE)										//满了返回-1
		return -1;
}

//查空白FAT，返回fat的下标，如果已经满了就返回-1
int findFreeFat() {
	int i = 3;
	for (i = 3; i < BLOCKNUM; i++)
		if (diskPtr->FAT[i] == 0)
			return i;
	if (i == BLOCKNUM)										//如果满了
		return -1;
}

//KMP模式匹配算法
void get_next(char*t, int *next)
{//-1 represents not exist, 0 repesents the longest proffix 
 //and suffix is 0 + 1 = 1, others represent ... others + 1
	int i, j;
	j = -1;
	next[0] = -1;
	for (i = 1; i < strlen(t); i++)
	{
		while (j > -1 && (t[j + 1] != t[i]))//如果不同
		{
			j = next[j];//j向前回溯 
		}
		if (t[j + 1] == t[i])//如果相同
		{
			j++;
		}
		next[i] = j;
	}
}

/*KMP*/
int KMP_index(char *S, char *T)
{
	if (!S || !T || S[0] == '\0' || T[0] == '\0')			//空指针或者空串
	{
		return -1;
	}
	int *next = new int[strlen(T)];
	get_next(T, next);										//得到next数组
	int j = -1;
	for (int i = 0; i < strlen(S); i++)
	{
		while (j > -1 && (T[j + 1] != S[i]))				//不匹配，k>-1表示有部分匹配
		{
			j = next[j];									//向前回溯
		}
		if (T[j + 1] == S[i])								//如果相同j++
		{
			j++;
		}
		if (T[j + 1] == '\0')								//如果子串比较到串尾，跳出
		{
			return i - strlen(T) + 1;
		}
	}
	return -1;
}

//根据原来的路径，分析得到它的目录指针，所在的盘块号，以及绝对路径
bool parsePath(char* orinPath, Ddirectory* parseToDir, int& toCurBlockId, string& toAbsPath)
{
	parseToDir = getCurrentDir();							//找到当前的目录
	toCurBlockId = currentBlockId;							//根据全局变量可以得到当前的currentBlockId
	toAbsPath = currentPath;								//和currentPath
	
	if (strcmp(orinPath, "/") == 0)							//直接返回根目录
	{
		toCurBlockId = 2;
		toAbsPath = "root/";
		return true;
	}

	char temp[45];
	int t = 0;

	for (int i = 0; i <= strlen(orinPath); i++)
	{
		if (orinPath[i] == '/' || orinPath[i] == '\0')				//遇到分隔符 
		{
			if (i == 0)
			{
				toCurBlockId = 2;
				toAbsPath = "root/";
			}
			else
			{
				temp[t] = '\0';
				if (strcmp(temp, "root/") == 0)					//绝对路径 
				{
					toCurBlockId = 2;
					toAbsPath = "root/";
				}
				else if (strcmp(temp, "..") == 0)				//返回上一级目录 
				{
					//已经在根目录的不能再向上返回
					if (strcmp(orinPath, "/") == 0 || strcmp(orinPath, "root") == 0 || strcmp(orinPath, "root/") == 0)
					{
						color(4);
						cout << "提示：已返回根目录！" << endl;
						color(16);
						return false;				//命令有错
					}
					else {//返回上一级目录 
						toCurBlockId = parseToDir->subFCB[0].upperBlockId;
						toAbsPath = toAbsPath.substr(0, toAbsPath.size() - strlen(parseToDir->subFCB[0].selfName) - 1);
						return true;
					}
				}
				else 									//进入子目录 
				{
					int tempSubDirFcbId = findSameSubDirName(parseToDir, temp);
					if (tempSubDirFcbId)				//如果找到了这个子目录
					{
						toCurBlockId = parseToDir->subFCB[tempSubDirFcbId].selfBlockId;
						toAbsPath = toAbsPath + parseToDir->subFCB[tempSubDirFcbId].selfName + "/";
					}
					else {								//要是没找到这个子目录
						return false;					//cd失败
					}
				}
			}
			parseToDir = getCurrentDir(toCurBlockId);
			t = 0;
		}
		else	temp[t++] = orinPath[i];					//没有遇到分隔符就按字符复制过去
	}
}

//根据绝对路径分析出最后的文件名
char* parseFileName(char* orinPath)
{
	int len = strlen(orinPath);
	int cnt = 0;
	while (len >= 0)
	{
		len--;
		if (orinPath[len] == '\\' || orinPath[len] == '/')
			break;
		cnt++;
	}len = strlen(orinPath);
	char* tempFileName = new char[cnt + 1];
	tempFileName[cnt] = '\0';
	while (cnt >= 0)
		tempFileName[--cnt] = orinPath[--len];
	return tempFileName;
}

//mkdir 为当前目录创建子目录，名称需要符合规范
bool mkDir(char* subDirName)
{
	loadFromDisk();																	//重新加载一下 
	Ddirectory *curDir = getCurrentDir();											//指向当前的目录 

	if (findSameSubDirName(curDir, subDirName)) {									//重名报错 如果find的返回值大于1说明有重名的
		color(4);
		cout << "错误：创建失败！当前目录下已有相同名的目录文件存在！" << endl;
		cout << "提示：请更换目录或重命名。" << endl;
		color(16);
		return false;																//创建失败！
	}
	int tempSubDirId = findFreeFcb(curDir);											//查找空闲文件控制块，存在tempSubDirId
	
	if (tempSubDirId == -1)															//满了提示信息
	{
		color(4);
		cout << "错误：创建失败！当前目录容量达到最大！" << endl;
		cout << "提示：请更换目录。" << endl;
		color(16);
		return false;																//创建失败！
	}
	int tempFatId = findFreeFat();													//如果当前目录不满，查空白FAT
	if (tempFatId == -1)															//如果盘块都被占用了
	{
		color(4);
		cout << "错误：创建失败！磁盘已满！" << endl;
		color(16);
		return false;																//创建失败！
	}

	//如果上面的问题都没有出现，就可以开始创建啦
	diskPtr->FAT[tempFatId] = 2;													//2表示目录文件
	strcpy(curDir->subFCB[tempSubDirId].selfName, subDirName);
	curDir->subFCB[tempSubDirId].selfType = DirType;
	curDir->subFCB[tempSubDirId].upperBlockId = currentBlockId;
	curDir->subFCB[tempSubDirId].selfBlockId = tempFatId;
	curDir->subFCB[tempSubDirId].initTime();
	curDir->subFCB[tempSubDirId].selfAttrib = ReadAndWrite;							//默认设置为可读写

	//初始化子目录文件的盘块
	curDir = (Ddirectory*)(diskPtr->data[tempFatId - 3]);							//指向创建的子目录的盘块
	curDir->initial(subDirName, currentBlockId, tempFatId);
	saveToDisk();																	//更新磁盘内容
	return true;																	//成功完成
}

//rmdir 删除当前目录下的文件夹
bool rmDir(char * delSubDirName)
{
	loadFromDisk();																//载入
	Ddirectory* curDir = getCurrentDir();										//当前目录的指针
	int delSubDirId = findSameSubDirName(curDir, delSubDirName);
	if (findSameSubDirName(curDir, delSubDirName))								//如果有相同名字的目录文件就可以删除
	{
		//进一步判断——目录文件是否为空
		int delFatId = curDir->subFCB[delSubDirId].selfBlockId;
		Ddirectory* subDir = (Ddirectory*)(diskPtr->data[delFatId - 3]);
		for (int i = 1; i < DIRSIZE; i++)
		{
			if (subDir->subFCB[i].selfType != Neither)
			{
				color(4);
				cout << "错误：删除失败！该目录文件不为空。" << endl;
				cout << "提示：请清空要删除目录文件下的子文件后再进行删除操作。" << endl;
				return false;
			}
		}
		//可以进行删除操作
		diskPtr->FAT[delFatId] = 0;												//置零，为空白
		char *temp = diskPtr->data[delFatId - 3];								//格式化子目录
		memset(temp, 0, BLOCKSIZE);
		curDir->subFCB[delSubDirId].initial();
		saveToDisk();															//更新至磁盘   注意：同步到磁盘的时机不要弄错了 
		return true;															//删除完成
	}
	else {
		color(4);
		cout << "错误：删除失败！该目录文件不存在。" << endl;
		cout << "提示：请核对文件名后再进行删除，可使用dir命令查看当前目录下的文件。" << endl;
		color(16);
		return false;								//删除失败
	}
}

//touch 创建文件
//说明：为了方便复制的操作，将载入和同步写在了主函数里
bool touch(char* subFileName, Ddirectory* curDir, int& tempSubFileFcbId, int& tempFatId)
{
	if (findSameSubFileName(curDir, subFileName)) {
		color(4);
		cout << "错误：失败！当前目录下已有同名文件存在！" << endl;
		cout << "提示：请更换目录。" << endl;
		color(16);
		return false; //创建失败！
	}
	tempSubFileFcbId = findFreeFcb(curDir);								//查找空闲文件控制块，存在tempSubDirId
	if (tempSubFileFcbId == -1)											//满了提示信息
	{
		color(4);
		cout << "错误：创建失败！当前目录容量达到最大！" << endl;
		cout << "提示：请更换目录。" << endl;
		color(16);
		return false; 													//创建失败！
	}
	tempFatId = findFreeFat();											//如果当前目录不满，查空白FAT
	if (tempFatId == -1)  												//如果盘块都被占用了
	{
		color(4);
		cout << "错误：创建失败！磁盘已满！" << endl;
		color(16);
		return false; 													//创建失败！
	}

	//如果上面的问题都没有出现，就可以开始创建啦
	diskPtr->FAT[tempFatId] = 1;										//1表示文件
	strcpy(curDir->subFCB[tempSubFileFcbId].selfName, subFileName);
	curDir->subFCB[tempSubFileFcbId].selfType = OFileType;
	curDir->subFCB[tempSubFileFcbId].upperBlockId = currentBlockId;
	curDir->subFCB[tempSubFileFcbId].selfBlockId = tempFatId;
	curDir->subFCB[tempSubFileFcbId].initTime();
	curDir->subFCB[tempSubFileFcbId].selfAttrib = ReadAndWrite;         //默认设置为可读写
	curDir->subFCB[tempSubFileFcbId].selfSize = 0;

	char* p = diskPtr->data[tempFatId - 3];
	memset(p, 4, BLOCKSIZE);											// Ctrl+D 用4初始化 以便后面查看的功能的判断条件 
	return true;														//成功完成
}

//echo 写文件内容
bool echo(char* subFileName)
{
	loadFromDisk();														//加载磁盘
	Ddirectory* curDir = getCurrentDir();
	int tempSubFileId = findSameSubFileName(curDir, subFileName);
	if (tempSubFileId == 0)												//没找到，文件不存在
	{
		color(4);
		cout << "错误：失败！文件不存在！" << endl;
		cout << "提示：请先创建文件后再进行写操作。" << endl;
		color(16);
		return false; //失败！
	}
	char* bgn = diskPtr->data[curDir->subFCB[tempSubFileId].selfBlockId - 3];
	char* end = diskPtr->data[curDir->subFCB[tempSubFileId].selfBlockId - 2];

	color(4);
	cout << "提示：请输入内容（以Ctrl + D组合键结束）。" << endl;
	color(16);

	char ch;
	while (ch = getchar())
	{
		if (ch == 4)			break;
		if (bgn < end) {
			*(bgn++) = ch;
		}
		else {
			*(bgn++) = 4;
			break;
		}
	}*bgn = 4;															//作为结束标志
	saveToDisk();														//同步数据
	return true;
}

//more 查看文件内容
bool more(char* subFileName)
{
	loadFromDisk();																	//载入
	Ddirectory* curDir = getCurrentDir();											//当前目录
	int tempSubFileId = findSameSubFileName(curDir, subFileName);
	if (tempSubFileId == 0)															//没找到，文件不存在
	{
		color(4);
		cout << "错误：失败！文件不存在！" << endl;
		cout << "提示：请先创建文件后再查看内容。" << endl;
		color(16);
		return false; //失败！
	}
	char* bgn = diskPtr->data[curDir->subFCB[tempSubFileId].selfBlockId - 3];
	char *end = diskPtr->data[curDir->subFCB[tempSubFileId].selfBlockId - 2];
	color(4);
	cout << "提示：文件内容如下。" << endl;
	color(16);
	while (bgn < end && (*bgn) != 4) cout << *(bgn++);
	cout << endl;
	saveToDisk();																	//同步
}

//find 在当前目录下寻找能匹配指定字符串的文件
bool find(char* str)
{
	loadFromDisk();
	Ddirectory *curDir = getCurrentDir();		//当前目录的指针
	bool flag = false;
	for (int i = 1; i < DIRSIZE; i++)
	{
		if (curDir->subFCB[i].selfType == OFileType)
		{
			//复制文件里的内容！这里要格外注意 
			char content[BLOCKSIZE];
			int cnt = 0;
			char* bgn = diskPtr->data[curDir->subFCB[i].selfBlockId - 3];
			char *end = diskPtr->data[curDir->subFCB[i].selfBlockId - 2];
			while (bgn < end && (*bgn) != 4){
				content[cnt++] = *(bgn++);
			} content[cnt] = '\0';
			if (KMP_index(content, str) > 0)
			{
				cout << curDir->subFCB[i].selfName << endl;
				flag = true;
			}
		}
	}
	if (!flag) {
		color(4);
		cout << "提示：未找到包含字符串" << str << "的文本文件。" << endl;
		color(16);
		return false;
	}
	saveToDisk();
}

//del 删除普通文件
bool del(char* delsubFileName)
{
	loadFromDisk();                       						 	//载入
	Ddirectory* curDir = getCurrentDir();   						//当前目录的指针
	int delSubFileId = findSameSubFileName(curDir, delsubFileName);
	if (delSubFileId)												//如果有相同名字的文件就可以删除
	{
		int delFatId = curDir->subFCB[delSubFileId].selfBlockId;	//要删除的文件所在的盘块号
		diskPtr->FAT[delFatId] = 0;									//置零，为空白
		char *temp = diskPtr->data[delFatId - 3];					//格式化
		memset(temp, 0, BLOCKSIZE);
		curDir->subFCB[delSubFileId].initial();
		saveToDisk();												//更新至磁盘   CHECK:注意，同步到磁盘的时机不要弄错了 
		return true;												//删除完成
	}
	else {
		color(4);
		cout << "错误：删除失败！该文件不存在。" << endl;
		cout << "提示：请核对文件名后再进行删除，可使用dir命令查看当前目录下的文件。" << endl;
		color(16);
		return false;								//删除失败
	}
}

//
bool attrib()
{
	loadFromDisk();											//重加载
	Ddirectory* curDir = getCurrentDir();					//得到当前目录的指针
	if (curDir->getSubNum())								//如果有文件
	{
		cout.setf(ios::left);								//靠左对齐 
		cout.width(14);
		cout << "文件属性";
		cout.width(14);
		cout << "文件名" << endl;
		for (int i = 1; i < DIRSIZE; i++)
		{
			if (curDir->subFCB[i].selfType != Neither)
			{
				cout.width(14);
				if (curDir->subFCB[i].selfAttrib == ReadOnly)
					cout << "只读";
				else if (curDir->subFCB[i].selfAttrib == WriteOnly)
					cout << "只写";
				else if (curDir->subFCB[i].selfAttrib == ReadAndWrite)
					cout << "读写";
				cout << currentPath << curDir->subFCB[i].selfName << endl;
			}
		}
	}
	saveToDisk();											//同步 
}

//dir list 查看当前目录下的文件。
bool dir()
{
	loadFromDisk();								//重加载
	Ddirectory* curDir = getCurrentDir();  		//得到当前目录的指针
	if (curDir->getSubNum())					//如果有文件
	{
		cout.setf(ios::left);					//靠左对齐 
		cout.width(14);
		cout << "文件名";
		cout.width(14);
		cout << "文件类型";
		cout.width(14);
		cout << "文件属性";
		cout.width(14);
		cout << "文件大小";
		cout.width(14);
		cout << "创建时间" << endl;
		for (int i = 1; i < DIRSIZE; i++)
		{
			if (curDir->subFCB[i].selfType != Neither)
			{
				cout.width(14);
				cout << curDir->subFCB[i].selfName;
				cout.width(14);
				if (curDir->subFCB[i].selfType == OFileType)
					cout << "FILE";
				else if (curDir->subFCB[i].selfType == DirType)
					cout << "DIR";
				cout.width(14);
				if (curDir->subFCB[i].selfAttrib == ReadOnly)
					cout << "只读";
				else if (curDir->subFCB[i].selfAttrib == WriteOnly)
					cout << "只写";
				else if (curDir->subFCB[i].selfAttrib == ReadAndWrite)
					cout << "读写";
				cout.width(14);
				if(curDir->subFCB[i].selfType == OFileType)
					cout << curDir->subFCB[i].getSize();
				else cout << "";
				curDir->subFCB[i].printTime();
			}			
		}
	}
	
	cout << "当前目录下共有" << curDir->getSubNum() << "项" << endl;
	cout << "              " << curDir->getSubFileNum() << "个文件" << endl;
	cout << "              " << curDir->getSubDirNum() << "个目录文件" << endl;
	saveToDisk();						//同步
}

//cd 改变工作路径，进入指定的路径下
bool cd(char *name)
{
	loadFromDisk();										//载入磁盘
	Ddirectory* curDir = getCurrentDir();				//初始化
	int toCurBlockId = currentBlockId;
	string toAbsPath = currentPath;
	parsePath(name, curDir, toCurBlockId, toAbsPath);	//根据原来的路径，分析得到它的目录指针，所在的盘块号，以及绝对路径
	currentBlockId = toCurBlockId;						//当前的盘块号替换为指定目录所在的盘块号
	currentPath = toAbsPath;							//当前的绝对路径替换为指定目录的绝对路径
}

//copy 将当前目录下指定的文件复制到指定路径下
bool copy(char *subFileName, char *toPath)
{
	loadFromDisk();																			//载入
	Ddirectory* curDir = getCurrentDir();
	Ddirectory* toDir = getCurrentDir();
	int tempSubFileId = findSameSubFileName(curDir, subFileName);
	if (tempSubFileId == 0)																	//没找到，文件不存在
	{
		color(4);
		cout << "错误：失败！文件不存在！" << endl;
		cout << "提示：请先创建文件后再复制。" << endl;
		color(16);
		return false; 																		//失败！
	}
	int temp1 = 0;																			//接收函数的两个值
	string temp2 = "";
	parsePath(toPath, toDir, temp1, temp2);													//进入to的目录下，创建一个文件，将内容写进去
	toDir = getCurrentDir(temp1);
	int tempSubFileFcbId = 0, tempFatId = 0;												//接收函数的两个值
	if(!touch(subFileName, toDir, tempSubFileFcbId, tempFatId))	return false;				//创建一个新文件 注意：可能会失败，失败需要退出！！！注意！ 
	curDir = getCurrentDir();																//！！！！！一定要回到当前文件夹！！！ 
	string content = diskPtr->data[curDir->subFCB[tempSubFileId].selfBlockId - 3];			//保存文件的内容 
	toDir->subFCB[tempSubFileFcbId].selfSize = content.length();						    //开辟空间 
	strcpy(diskPtr->data[tempFatId - 3], content.c_str());									//把文件的内容复制进去 

	saveToDisk();
}

//cpdir 将当前路径下的空目录复制到指定路径下
bool cpdir(char *subDirName, char *toPath)
{
	loadFromDisk();																			//载入
	Ddirectory* curDir = getCurrentDir();
	int tempSubDirId = findSameSubDirName(curDir, subDirName);
	if (tempSubDirId == 0)																	//没找到，文件不存在
	{
		color(4);
		cout << "错误：失败！文件不存在！" << endl;
		cout << "提示：请先创建文件后再复制。" << endl;
		color(16);
		return false;
	}
	char* temp = new char[currentPath.length()+1];
	strcpy(temp, currentPath.substr(4).c_str());
	cd(toPath);
	if (!mkDir(subDirName)){
		cd(temp);
		return false;
	}
	cd(temp);
	saveToDisk();
	return true;
}

//move 将当前目录下指定的文件移动到指定路径下
bool move(char *subFilename, char* toPath){
	loadFromDisk();																			//加载
	copy(subFilename, toPath);																//先复制过去
	del(subFilename);																		//复制成功后再删除原来的文件
	saveToDisk();																			//保存
	return true;
}

//rename 为当前目录下的指定文件修改名字
bool rename(char* oldName, char* newName)
{
	loadFromDisk();
	Ddirectory *curDir = getCurrentDir();
	int tempId = findSameSubFileName(curDir, oldName);										//找同名文件
	if (tempId == 0)
		tempId = findSameSubDirName(curDir, oldName);										//找同名目录
	if (tempId == 0)																		//如果都没有，返回false
	{
		color(4);
		cout << "当前目录下没有名为" << oldName << "的文件或目录" << endl;
		color(16);
		return false;
	}
	else {
		strcpy(curDir->subFCB[tempId].selfName, newName);									//如果找到了就重命名
	}
	saveToDisk();
	return true;
}

//从本地磁盘引入文件——引入到虚拟盘上
bool importFormDisk(char *realDiskFile, char *virtualDiskFilePath)
{
	loadFromDisk();																			//加载
	FILE* f;
	if ((f = fopen(realDiskFile, "r")) == NULL)
	{
		color(4);
		cout << "提示：本地磁盘的文件不存在！" << endl;
		color(16);
		return false;
	}
	//如果本地磁盘的文件存在，先读内容，存在content里面
	char content[BLOCKSIZE],ch;
	memset(content, 4, BLOCKSIZE);
	int i = 0;
	while ((ch = getc(f)) != EOF) {
		content[i++] = ch;
		if (i >= BLOCKSIZE) {
			color(4);
			cout << "错误：文件过大，无法导入。" << endl;
			color(16);
			return false;
		}
	}content[i] = 4;

	Ddirectory* toDir = getCurrentDir();													//要去到的目录，以当前目录初始化
	int temp1 = 0;																			//接收函数的两个值
	string temp2 = "";
	parsePath(virtualDiskFilePath, toDir, temp1, temp2);									//进入to的目录下，创建一个文件，将内容写进去
	toDir = getCurrentDir(temp1);
	int tempSubFileFcbId = 0, tempFatId = 0;												//接收函数的两个值
	if (!touch(parseFileName(realDiskFile), toDir, tempSubFileFcbId, tempFatId))
		return false;																		//创建一个新文件~   CHECK：可能会失败，失败需要退出！！！注意！ 
	toDir->subFCB[tempSubFileFcbId].selfSize = strlen(content);								//开辟空间 
	strcpy(diskPtr->data[tempFatId - 3], content);											//把文件的内容复制进去 
	saveToDisk();																			//保存
	return true;
}

//从当前目录下导出文件至本地磁盘
bool exportToDisk(char *virtualDiskFile, char *realDiskFilePath)
{
	loadFromDisk();																			//加载
	Ddirectory *curDir = getCurrentDir();													//当前目录
	//检查当前目录下是否存在这个文件
	int tempSubFileId = findSameSubFileName(curDir, virtualDiskFile);
	if (!tempSubFileId) {
		color(4);
		cout << "错误：当前目录下不存在名为" << virtualDiskFile << "的文件，无法导出。" << endl;
		cout << "提示：请核对文件名后再导出。" << endl;
		color(16);
		return false;
	}
	//如果存在的话
	FILE* f;
	string temp1 = realDiskFilePath, temp2 = virtualDiskFile;
	if ((f = fopen((temp1 + '\\' + temp2).c_str(), "w")) == NULL)							//windows下用\来分割
	{
		color(4);
		cout << "错误：无法导出至本地磁盘的指定目录下，请检查路径。" << endl;
		color(16);
		return false;
	}
	int tempFileBlockId = curDir->subFCB[tempSubFileId].selfBlockId;
	//保存文件内容
	char *content = new char[strlen(diskPtr->data[tempFileBlockId - 3]) + 1];
	strcpy(content, diskPtr->data[tempFileBlockId - 3]);
	while (*(content) != 4) {
		fputc(*content++, f);
	}fclose(f);
	saveToDisk();																			//保存
	return true;
}

//xcopy 将当前路径下的目录树复制到指定路径下
bool xcopy(char* dirName, char* toPath)
{
	Ddirectory *curDir = getCurrentDir();							//当前目录
	if (curDir->getSubNum() > 0) {									//如果不是空目录就要一级一级的复制过去
		cpdir(dirName, toPath);
		Ddirectory* parseToDir;
		int toCurBlockId = 0;
		string toAbsPath = ""; 
		parsePath(toPath, parseToDir, toCurBlockId, toAbsPath);
		char* toPathch = new char[toAbsPath.length() + strlen(dirName) + 1 - 4];
		strcpy(toPathch, toAbsPath.substr(4).c_str());
		strcat(toPathch, dirName);
		cd(dirName);
		curDir = getCurrentDir();									//当前目录
		int i = 1;
		for (; i < DIRSIZE; i++)
		{
			if (curDir->subFCB[i].selfType == OFileType)
				copy(curDir->subFCB[i].selfName, toPathch);
			else if (curDir->subFCB[i].selfType == DirType)
				xcopy(curDir->subFCB[i].selfName, toPathch);		//递归！深度优先！
		}
		if (i == DIRSIZE){											//搜索完一个目录文件后要回退到上一级
			char* deli = new char[3];
			deli[0] = '.';
			deli[1] = '.';
			deli[2] = '\0';
			cd(deli);
		}
	}else{
		cpdir(dirName, toPath);
	}
}

//help，打印帮助信息
void helpPrint()
{
	cout << "------help------" << endl;
	cout << "dir                      查看当前目录下的文件" << endl;
	cout << "mkdir   [目录名]         在当前目录下创建目录文件 " << endl;
	cout << "rmdir   [目录名]         在当前目录下删除目录文件 " << endl;
	cout << "cd      [目录名]         进入下级目录或返回上级目录 " << endl;
	cout << "del     [文件名]         在当前目录下删除普通文件 " << endl;
	cout << "touch   [文件名]         在当前目录下创建文本文件 " << endl;
	cout << "echo    [文件名]         对当前目录下指定文本文件重写入内容 " << endl;
	cout << "more    [文件名]         显示当前目录下指定文本文件的内容 " << endl;
	cout << "rename  [文件名/目录名]  为当前目录下的指定文件修改名字" << endl;
	cout << "move    [文件名][路径]   将当前目录下指定的文件移动到指定路径下" << endl;
	cout << "copy    [文件名][路径]   将当前目录下指定的文件复制到指定路径下" << endl;
	cout << "cpdir   [目录名]         将当前路径下的空目录复制到指定路径下" << endl;
	cout << "xcopy   [目录名]         将当前路径下的目录复制到指定路径下" << endl;
	cout << "import  [路径][路径]     从本地磁盘引入文件" << endl;
	cout << "export  [文件名][路径]   从当前目录下导出文件至本地磁盘" << endl;
	cout << "attrib                   显示看当前目录下文件的属性" << endl;					
	cout << "find    [字符串]         在当前目录下寻找能匹配指定字符串的文件" << endl;
	cout << "clear                    清除屏幕" << endl;
	cout << "time                     显示系统时间" << endl;
	cout << "exit                     退出文件系统" << endl;
}

//clear 清除屏幕
void clear()
{
	system("cls");
}

//打印当前时间
void timePrint()
{
	time_t timep;
	time(&timep);
	struct tm *p = localtime(&timep);
	cout << 1900 + p->tm_year << "/" <<  p->tm_mon + 1 << "/" << p->tm_mday << " ";
	cout << p->tm_hour << ":" << p->tm_min << ":" << p->tm_sec<<endl;
}

//退出 
bool exitSys()
{
	saveToDisk();													//保存所有文件后退出，释放内存
	free(diskPtr);													//CHECK:不要重复释放filePtr！ 
	return true;
}

int main()
{
	char temp1[20], temp2[30];										//接受命令的参数
	startAddress = (char *)malloc(DISKSIZE);						//申请虚拟空间并且初始化
	diskPtr = (Disk *)(startAddress);								//虚拟磁盘初始化
	//读入虚拟文件系统的信息
	//如果不能加载，则需要格式化。指的是第一次创建VFS的时候
	if (!loadFromDisk()){
		welcomeInterface();
		initialVFS();
	}cout << "提示：输入help可查看帮助信息。" << endl;
	//已经加载好啦,开始输入命令吧
	while (true)
	{
		cout << ">" << currentPath;									//系统提示符 + 当前路径
		cin >> inpCmd;
		if (inpCmd == "mkdir") {
			cin >> temp1;
			mkDir(temp1);
		}else if (inpCmd == "rmdir") {
			cin >> temp1;
			rmDir(temp1);
		}else if (inpCmd == "dir") {
			dir();
		}else if (inpCmd == "touch") {
			cin >> temp1;
			loadFromDisk();
			int a1 = 0, a2 = 0;
			Ddirectory* tempDir = getCurrentDir();
			touch(temp1, tempDir, a1, a2);
			saveToDisk();
		}else if (inpCmd == "more") {
			cin >> temp1;
			more(temp1);
		}else if (inpCmd == "echo") {
			cin >> temp1;
			echo(temp1);
		}else if (inpCmd == "del") {
			cin >> temp1;
			del(temp1);
		}else if(inpCmd == "move"){
			cin>>temp1>>temp2;
			move(temp1, temp2);
		}
		else if (inpCmd == "copy") {
			cin >> temp1 >> temp2;
			copy(temp1, temp2);
		}
		else if (inpCmd == "rename") {
			cin >> temp1 >> temp2;
			rename(temp1, temp2);
		}
		else if (inpCmd == "attrib") {
			attrib();
		}
		else if (inpCmd == "cpdir") {
			cin >> temp1 >> temp2;
			cpdir(temp1, temp2);
		}
		else if (inpCmd == "xcopy") {
			cin >> temp1 >> temp2;
			loadFromDisk();
			xcopy(temp1, temp2);
			saveToDisk();
		}
		else if (inpCmd == "clear") {
			clear();
		}
		else if (inpCmd == "help") {
			helpPrint();
		}
		else if (inpCmd == "cd") {
			cin >> temp1;
			cd(temp1);		
		}else if (inpCmd == "find") {
			cin >> temp1;
			find(temp1);
		}else if(inpCmd == "import") {
			cin >> temp1 >> temp2;
			importFormDisk(temp1, temp2);
		}else if(inpCmd == "export") {
			cin >> temp1 >> temp2;
			exportToDisk(temp1, temp2);
		}else if (inpCmd == "time") {
			timePrint();
		}else if (inpCmd == "exit") {
			exitSys();
			exit(0);
		}else {
			color(4);
			cout << "命令格式有误，请重新输入。" << endl;
			color(16);
		}
	}
	system("pause");
	return 0;
}
