#include<cstdio>
#include<iostream>
#include<fstream>
#include<string>
#include<cstring>
#include <windows.h>
#include<conio.h>
#include<ctime>
#include<cstdlib>

#define BLOCKSIZE 1024							    //�̿��СΪ1024 = 1KB
#define BLOCKNUM 128								//�̿�����
#define DISKSIZE BLOCKSIZE*BLOCKNUM					//���̴�С 1024*128
#define DIRSIZE BLOCKSIZE/sizeof(FileControlBlock)  //Ŀ¼�ļ�����

using namespace std;

/*������������������������ö�����Ͷ��塪��������������������������*/
//�ļ�����
enum FileAttrib {
	ReadOnly = 1,     //ֻ��
	WriteOnly = 2,    //ֻд
	ReadAndWrite = 3  //��д
};

//�ļ�����
enum FileType {
	Neither,
	OFileType = 1, //�ļ�����
	DirType = 2    //Ŀ¼����
};

/*�������������������������ṹ�嶨�塪��������������������������*/
//ʱ��
typedef struct DateTime {
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
}DateTime;

struct FileControlBlock //�ļ����ƿ� FCB
{
	char selfName[20];               //�ļ�����
	FileAttrib selfAttrib;           //�ļ�����
	FileType selfType;               //�ļ�����
	DateTime createTime;             //����ʱ��
	int selfSize;                    //�ļ���С
	int upperBlockId;                //��һ�����̿��
	int selfBlockId;                 //������̿��

	void initial();
	void initTime();
	void printTime();
	int getSize();
};

//��ʼ���ļ����ƿ�
void FileControlBlock::initial()
{
	selfName[0] = '\0';
	selfAttrib = ReadAndWrite;						//Ĭ�Ͽɶ���д
	selfType = Neither;
	selfSize = upperBlockId = selfBlockId = 0;
}

//��ʼ���ļ����ƿ�Ĵ���ʱ�䡪���Ե�ǰʱ����Ϊ����ʱ��
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

//��ӡ�ļ��Ĵ���ʱ��
void FileControlBlock::printTime(){
	cout << createTime.year << "/" << createTime.month << "/" << createTime.day << " ";
	cout << createTime.hour << ":" << createTime.minute << ":" << createTime.second <<endl;
}

//Ŀ¼
struct Ddirectory
{
	FileControlBlock subFCB[DIRSIZE];
	int subDirNum;                          //��Ŀ¼������
	int subFileNum;                         //���ļ�������
	int subNum;                             //���ļ�����Ŀ¼��������

	//���ڵ�ǰĿ¼��˵���������֣���һ��Ŀ¼�Ŀ�ţ��������ڵĿ��
	void initial(char* selfName, int upperBlockId, int selfBlockId);
	int getSubDirNum();
	int getSubFileNum();                         //���ļ�������
	int getSubNum();
};

//��ʼ��һ����Ŀ¼
void Ddirectory::initial(char* selfName, int upperBlockId, int selfBlockId)
{
	//�Ƚ���Ŀ¼��ʽ����0��λ�����´洢��Ŀ¼����Ϣ
	for (int i = 1; i < DIRSIZE; i++)
	{
		subFCB[i].selfType = Neither;             //��Ŀ¼�������ļ�����Ŀ¼
		subFCB[i].upperBlockId = selfBlockId;     //��Ŀ¼��id��sub����һ����id��
	}

	//���ø�Ŀ¼������Ϣ��������0��λ
	subFCB[0].selfSize = 0;
	subFCB[0].selfBlockId = selfBlockId;
	subFCB[0].upperBlockId = upperBlockId;
	strcpy(subFCB[0].selfName, selfName);
	subFCB[0].initTime();

	//��Ŀ¼�������ļ�����Ŀ¼
	subDirNum = subFileNum = subNum = 0;
}

//�õ���ǰĿ¼��Ŀ¼�ļ�������
int Ddirectory::getSubDirNum() {
	int cnt = 0;
	for (int i = 1; i < DIRSIZE; i++)
		if (subFCB[i].selfType == DirType)	cnt++;
	return cnt;
}

//�õ���ǰĿ¼����ͨ�ļ�������
int Ddirectory::getSubFileNum() {
	int cnt = 0;
	for (int i = 1; i < DIRSIZE; i++)
		if (subFCB[i].selfType == OFileType)	cnt++;
	return cnt;
}

//�õ���ǰĿ¼���ļ�������
int Ddirectory::getSubNum() {
	return getSubDirNum() + getSubFileNum();
}

//Disk�ṹ
struct Disk
{
	Ddirectory root;                      //��Ŀ¼
	int FAT[BLOCKNUM], FAT2[BLOCKNUM];    // FAT
	char data[BLOCKNUM - 3][BLOCKSIZE];   //���ݿ�����
	void initial();
};

//��ʼ������
void Disk::initial()
{
	memset(FAT, 0, BLOCKNUM);
	memset(FAT, 0, BLOCKNUM);
	memset(data, 0, sizeof(data));
	FAT[0] = FAT[1] = -2;
	root.initial((char*)"root", 2, 2);
}

/*������������������������ȫ�ֱ����Ķ��塪��������������������������*/
FILE *filePtr;                                   //�ļ�ָ�� VFS
string currentPath = "root/";                    //��ǰ·��
int currentBlockId = 2;                          //��ǰĿ¼���ڵ��̿��
const char* VirtualDisk = "VirtualDisk.bin";     //������  !!!�����const��Ϊ�˷���ʹ��fopen��������ʹ��char*
string inpCmd;                                   //�����command
Disk *diskPtr;                                   //������̵Ķ���
char* startAddress;                              //��������׵�ַ      

//�õ���ͨ�ļ��Ĵ�С
int FileControlBlock::getSize() {
	int i = 0;
	for (; i < BLOCKSIZE; i++)
	{
		if (diskPtr->data[selfBlockId - 3][i] == 4)
			break;
	}
	return i;
}   

/*�������������ṹ�庯����������������ͳһ������������ҡ���������*/
/*��ɫ*/ 
void color(short x);//�Զ��庯���ݲ����ı���ɫ
//����������
bool saveToDisk();
//�Ӵ��̼������ڴ�
bool loadFromDisk();
//��ʽ���Ļ�ӭ����
void welcomeInterface();
//��ʼ��VFS
void initialVFS();
//���ص�ǰĿ¼��ָ��
/*---------------------------��������������---------------------*/ 
Ddirectory* getCurrentDir();
//��ǰĿ¼������Ŀ¼�ļ��ļ��  ���������ļ������±�
//Ѱ�ҵ�ǰĿ¼���Ƿ����subDirName���ֵ�Ŀ¼�ļ�
int findSameSubDirName(Ddirectory* curDir, char* subDirName);
//�鵱ǰĿ¼�µ������ļ������������±꣬���򷵻�0
int findSameSubFileName(Ddirectory* curDir, char* subFileName);
//���ҿ����ļ����ƿ飬����i������Ѿ����˾ͷ���-1
int findFreeFcb(Ddirectory* curDir);
//��հ�FAT������fat���±꣬����Ѿ����˾ͷ���-1
int findFreeFat(); 
//����ԭ����·���������õ�����Ŀ¼ָ�룬���ڵ��̿�ţ��Լ�����·��
bool parsePath(char* orinPath, Ddirectory* parseToDir, int& toCurBlockId, string& toAbsPath); 
//���ݾ���·�������������ļ���
char* parseFileName(char* orinPath);

/*-------------���ܺ���������------------*/ 
//mkdir Ϊ��ǰĿ¼������Ŀ¼��������Ҫ���Ϲ淶
bool mkDir(char* subDirName);
//rmdir ɾ����ǰĿ¼�µ��ļ���
bool rmDir(char * delSubDirName);
//touch �����ļ�
//˵����Ϊ�˷��㸴�ƵĲ������������ͬ��д������������
bool touch(char* subFileName, Ddirectory* curDir, int& tempSubFileFcbId, int& tempFatId);
//echo д�ļ�����
bool echo(char* subFileName);
//more �鿴�ļ�����
bool more(char* subFileName);
//del ɾ����ͨ�ļ�
bool del(char* delsubFileName);
//dir list
bool dir();
//cd �ı乤��·���������¼�Ŀ¼���߷����ϼ�Ŀ¼
//CHECK�����ÿһ���̸߳ı乤��·��
bool cd(char *name);
bool copy(char *subFileName, char *toPath);//����Ҫ����
//�˳�
bool exitSys();
//help����ӡ������Ϣ
void helpPrint();

/*������������������������̨��ɫ���á�����������������������*/
void color(short x) //�Զ��庯���ݲ����ı���ɫ   
{
	if (x >= 0 && x <= 15)//������0-15�ķ�Χ��ɫ  
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), x);    //ֻ��һ���������ı�������ɫ   
	else//Ĭ�ϵ���ɫ��ɫ  
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

/*������������������������̵Ķ�д��������������������������*/
//����������
bool saveToDisk()
{
	filePtr = fopen(VirtualDisk, "wb+");
	if (filePtr != NULL) //wb+��д�򿪶������ļ�
	{
		fwrite(startAddress, sizeof(char), DISKSIZE, filePtr);
		fclose(filePtr);
		return true;
	}
	else return false;
}

//�Ӵ��̼������ڴ�
bool loadFromDisk()
{
	filePtr = fopen(VirtualDisk, "rb");
	if (filePtr != NULL)  //rb������ֻ����ʽ���ļ�
	{
		fread(startAddress, sizeof(char), DISKSIZE, filePtr);
		return true;
	}
	else return false;
}

//��ʽ���Ļ�ӭ����
void welcomeInterface()
{
	cout << "********************************************************" << endl;
	cout << "******                                            ******" << endl;
	cout << "******------------��ӭ���������ļ�ϵͳ------------******" << endl;
	cout << "******                                            ******" << endl;
	cout << "********************************************************" << endl;
}

//��ʼ��VFS
void initialVFS()
{
	cout << endl;
	cout << "��Ҫ��ʽ�������ļ�ϵͳ���������ݽ����������";
	color(4);		//��ɫ
	cout << "����";
	color(16);		//�ָ�Ĭ����ɫ
	cout << "��\n��ѡ��[y/Y]����������[n/N]�˳���\n�����룺";
	while (true)
	{
		char type = getchar();
		if (type == 'y' || type == 'Y')						//���ѡ��y/N�ͽ��г�ʼ��
		{
			cout << "��ʽ�������ļ�ϵͳ��..." << endl;
			currentBlockId = 2; //root
			currentPath = "root/";
			diskPtr->initial();								//���ָ̻���ʼ״̬
			saveToDisk();									//��ʼ�ú󱣴�������
			//���г�ʼ�����˳�while�����ѭ��
			cout << "��ʽ����ɡ�" << endl;
			break;
		}
		else if (type == 'n' || type == 'N')
			exit(0);
		else
			cout << "��ʽ������ʹ��[y/Y]����[n/N]����ѡ��������:";
	}
}

//����BlockIdĿ¼��ָ�룬Ĭ�Ϸ��ص�ǰĿ¼��ָ��
Ddirectory* getCurrentDir()
{
	Ddirectory *curDir;											//���ص�ǰĿ¼��ָ��
	if (currentBlockId == 2)
		curDir = &(diskPtr->root);
	else
		curDir = (Ddirectory*)(diskPtr->data[currentBlockId - 3]);
	return curDir;
}

//��������ĺ��� 
Ddirectory* getCurrentDir(int BlockId)
{
	Ddirectory *curDir;											//����blockIDĿ¼��ָ��
	if (BlockId == 2)
		curDir = &(diskPtr->root);
	else
		curDir = (Ddirectory*)(diskPtr->data[BlockId - 3]);
	return curDir;
}

//��ǰĿ¼������Ŀ¼�ļ��ļ��  ���������ļ������±꣬���򷵻�0
//Ѱ�ҵ�ǰĿ¼���Ƿ����subDirName���ֵ�Ŀ¼�ļ�
int findSameSubDirName(Ddirectory* curDir, char* subDirName)
{
	for (int i = 1; i < DIRSIZE; i++)
		if ((curDir->subFCB[i].selfType == DirType) && (strcmp(subDirName, curDir->subFCB[i].selfName) == 0))
			//CHECK:�ж���Ϣһ��ʼд���ˡ�����һ����DirType��һ������strcmp
			return i;
	return 0;
}

//�鵱ǰĿ¼�µ������ļ������������±꣬���򷵻�0
int findSameSubFileName(Ddirectory* curDir, char* subFileName)
{
	for (int i = 1; i < DIRSIZE; i++)
		if ((curDir->subFCB[i].selfType == OFileType) && (strcmp(subFileName, curDir->subFCB[i].selfName) == 0))
			return i;
	return 0;
}

//���ҿ����ļ����ƿ飬����i������Ѿ����˾ͷ���-1
int findFreeFcb(Ddirectory* curDir)
{
	int i = 1;
	for (i = 1; i < DIRSIZE; i++)
		if (curDir->subFCB[i].selfType == Neither) 			//�Ȳ���Ŀ¼�ļ�Ҳ�����ı��ļ���Ϊ��
			return i;
	if (i == DIRSIZE)										//���˷���-1
		return -1;
}

//��հ�FAT������fat���±꣬����Ѿ����˾ͷ���-1
int findFreeFat() {
	int i = 3;
	for (i = 3; i < BLOCKNUM; i++)
		if (diskPtr->FAT[i] == 0)
			return i;
	if (i == BLOCKNUM)										//�������
		return -1;
}

//KMPģʽƥ���㷨
void get_next(char*t, int *next)
{//-1 represents not exist, 0 repesents the longest proffix 
 //and suffix is 0 + 1 = 1, others represent ... others + 1
	int i, j;
	j = -1;
	next[0] = -1;
	for (i = 1; i < strlen(t); i++)
	{
		while (j > -1 && (t[j + 1] != t[i]))//�����ͬ
		{
			j = next[j];//j��ǰ���� 
		}
		if (t[j + 1] == t[i])//�����ͬ
		{
			j++;
		}
		next[i] = j;
	}
}

/*KMP*/
int KMP_index(char *S, char *T)
{
	if (!S || !T || S[0] == '\0' || T[0] == '\0')			//��ָ����߿մ�
	{
		return -1;
	}
	int *next = new int[strlen(T)];
	get_next(T, next);										//�õ�next����
	int j = -1;
	for (int i = 0; i < strlen(S); i++)
	{
		while (j > -1 && (T[j + 1] != S[i]))				//��ƥ�䣬k>-1��ʾ�в���ƥ��
		{
			j = next[j];									//��ǰ����
		}
		if (T[j + 1] == S[i])								//�����ͬj++
		{
			j++;
		}
		if (T[j + 1] == '\0')								//����Ӵ��Ƚϵ���β������
		{
			return i - strlen(T) + 1;
		}
	}
	return -1;
}

//����ԭ����·���������õ�����Ŀ¼ָ�룬���ڵ��̿�ţ��Լ�����·��
bool parsePath(char* orinPath, Ddirectory* parseToDir, int& toCurBlockId, string& toAbsPath)
{
	parseToDir = getCurrentDir();							//�ҵ���ǰ��Ŀ¼
	toCurBlockId = currentBlockId;							//����ȫ�ֱ������Եõ���ǰ��currentBlockId
	toAbsPath = currentPath;								//��currentPath
	
	if (strcmp(orinPath, "/") == 0)							//ֱ�ӷ��ظ�Ŀ¼
	{
		toCurBlockId = 2;
		toAbsPath = "root/";
		return true;
	}

	char temp[45];
	int t = 0;

	for (int i = 0; i <= strlen(orinPath); i++)
	{
		if (orinPath[i] == '/' || orinPath[i] == '\0')				//�����ָ��� 
		{
			if (i == 0)
			{
				toCurBlockId = 2;
				toAbsPath = "root/";
			}
			else
			{
				temp[t] = '\0';
				if (strcmp(temp, "root/") == 0)					//����·�� 
				{
					toCurBlockId = 2;
					toAbsPath = "root/";
				}
				else if (strcmp(temp, "..") == 0)				//������һ��Ŀ¼ 
				{
					//�Ѿ��ڸ�Ŀ¼�Ĳ��������Ϸ���
					if (strcmp(orinPath, "/") == 0 || strcmp(orinPath, "root") == 0 || strcmp(orinPath, "root/") == 0)
					{
						color(4);
						cout << "��ʾ���ѷ��ظ�Ŀ¼��" << endl;
						color(16);
						return false;				//�����д�
					}
					else {//������һ��Ŀ¼ 
						toCurBlockId = parseToDir->subFCB[0].upperBlockId;
						toAbsPath = toAbsPath.substr(0, toAbsPath.size() - strlen(parseToDir->subFCB[0].selfName) - 1);
						return true;
					}
				}
				else 									//������Ŀ¼ 
				{
					int tempSubDirFcbId = findSameSubDirName(parseToDir, temp);
					if (tempSubDirFcbId)				//����ҵ��������Ŀ¼
					{
						toCurBlockId = parseToDir->subFCB[tempSubDirFcbId].selfBlockId;
						toAbsPath = toAbsPath + parseToDir->subFCB[tempSubDirFcbId].selfName + "/";
					}
					else {								//Ҫ��û�ҵ������Ŀ¼
						return false;					//cdʧ��
					}
				}
			}
			parseToDir = getCurrentDir(toCurBlockId);
			t = 0;
		}
		else	temp[t++] = orinPath[i];					//û�������ָ����Ͱ��ַ����ƹ�ȥ
	}
}

//���ݾ���·�������������ļ���
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

//mkdir Ϊ��ǰĿ¼������Ŀ¼��������Ҫ���Ϲ淶
bool mkDir(char* subDirName)
{
	loadFromDisk();																	//���¼���һ�� 
	Ddirectory *curDir = getCurrentDir();											//ָ��ǰ��Ŀ¼ 

	if (findSameSubDirName(curDir, subDirName)) {									//�������� ���find�ķ���ֵ����1˵����������
		color(4);
		cout << "���󣺴���ʧ�ܣ���ǰĿ¼��������ͬ����Ŀ¼�ļ����ڣ�" << endl;
		cout << "��ʾ�������Ŀ¼����������" << endl;
		color(16);
		return false;																//����ʧ�ܣ�
	}
	int tempSubDirId = findFreeFcb(curDir);											//���ҿ����ļ����ƿ飬����tempSubDirId
	
	if (tempSubDirId == -1)															//������ʾ��Ϣ
	{
		color(4);
		cout << "���󣺴���ʧ�ܣ���ǰĿ¼�����ﵽ���" << endl;
		cout << "��ʾ�������Ŀ¼��" << endl;
		color(16);
		return false;																//����ʧ�ܣ�
	}
	int tempFatId = findFreeFat();													//�����ǰĿ¼��������հ�FAT
	if (tempFatId == -1)															//����̿鶼��ռ����
	{
		color(4);
		cout << "���󣺴���ʧ�ܣ�����������" << endl;
		color(16);
		return false;																//����ʧ�ܣ�
	}

	//�����������ⶼû�г��֣��Ϳ��Կ�ʼ������
	diskPtr->FAT[tempFatId] = 2;													//2��ʾĿ¼�ļ�
	strcpy(curDir->subFCB[tempSubDirId].selfName, subDirName);
	curDir->subFCB[tempSubDirId].selfType = DirType;
	curDir->subFCB[tempSubDirId].upperBlockId = currentBlockId;
	curDir->subFCB[tempSubDirId].selfBlockId = tempFatId;
	curDir->subFCB[tempSubDirId].initTime();
	curDir->subFCB[tempSubDirId].selfAttrib = ReadAndWrite;							//Ĭ������Ϊ�ɶ�д

	//��ʼ����Ŀ¼�ļ����̿�
	curDir = (Ddirectory*)(diskPtr->data[tempFatId - 3]);							//ָ�򴴽�����Ŀ¼���̿�
	curDir->initial(subDirName, currentBlockId, tempFatId);
	saveToDisk();																	//���´�������
	return true;																	//�ɹ����
}

//rmdir ɾ����ǰĿ¼�µ��ļ���
bool rmDir(char * delSubDirName)
{
	loadFromDisk();																//����
	Ddirectory* curDir = getCurrentDir();										//��ǰĿ¼��ָ��
	int delSubDirId = findSameSubDirName(curDir, delSubDirName);
	if (findSameSubDirName(curDir, delSubDirName))								//�������ͬ���ֵ�Ŀ¼�ļ��Ϳ���ɾ��
	{
		//��һ���жϡ���Ŀ¼�ļ��Ƿ�Ϊ��
		int delFatId = curDir->subFCB[delSubDirId].selfBlockId;
		Ddirectory* subDir = (Ddirectory*)(diskPtr->data[delFatId - 3]);
		for (int i = 1; i < DIRSIZE; i++)
		{
			if (subDir->subFCB[i].selfType != Neither)
			{
				color(4);
				cout << "����ɾ��ʧ�ܣ���Ŀ¼�ļ���Ϊ�ա�" << endl;
				cout << "��ʾ�������Ҫɾ��Ŀ¼�ļ��µ����ļ����ٽ���ɾ��������" << endl;
				return false;
			}
		}
		//���Խ���ɾ������
		diskPtr->FAT[delFatId] = 0;												//���㣬Ϊ�հ�
		char *temp = diskPtr->data[delFatId - 3];								//��ʽ����Ŀ¼
		memset(temp, 0, BLOCKSIZE);
		curDir->subFCB[delSubDirId].initial();
		saveToDisk();															//����������   ע�⣺ͬ�������̵�ʱ����ҪŪ���� 
		return true;															//ɾ�����
	}
	else {
		color(4);
		cout << "����ɾ��ʧ�ܣ���Ŀ¼�ļ������ڡ�" << endl;
		cout << "��ʾ����˶��ļ������ٽ���ɾ������ʹ��dir����鿴��ǰĿ¼�µ��ļ���" << endl;
		color(16);
		return false;								//ɾ��ʧ��
	}
}

//touch �����ļ�
//˵����Ϊ�˷��㸴�ƵĲ������������ͬ��д������������
bool touch(char* subFileName, Ddirectory* curDir, int& tempSubFileFcbId, int& tempFatId)
{
	if (findSameSubFileName(curDir, subFileName)) {
		color(4);
		cout << "����ʧ�ܣ���ǰĿ¼������ͬ���ļ����ڣ�" << endl;
		cout << "��ʾ�������Ŀ¼��" << endl;
		color(16);
		return false; //����ʧ�ܣ�
	}
	tempSubFileFcbId = findFreeFcb(curDir);								//���ҿ����ļ����ƿ飬����tempSubDirId
	if (tempSubFileFcbId == -1)											//������ʾ��Ϣ
	{
		color(4);
		cout << "���󣺴���ʧ�ܣ���ǰĿ¼�����ﵽ���" << endl;
		cout << "��ʾ�������Ŀ¼��" << endl;
		color(16);
		return false; 													//����ʧ�ܣ�
	}
	tempFatId = findFreeFat();											//�����ǰĿ¼��������հ�FAT
	if (tempFatId == -1)  												//����̿鶼��ռ����
	{
		color(4);
		cout << "���󣺴���ʧ�ܣ�����������" << endl;
		color(16);
		return false; 													//����ʧ�ܣ�
	}

	//�����������ⶼû�г��֣��Ϳ��Կ�ʼ������
	diskPtr->FAT[tempFatId] = 1;										//1��ʾ�ļ�
	strcpy(curDir->subFCB[tempSubFileFcbId].selfName, subFileName);
	curDir->subFCB[tempSubFileFcbId].selfType = OFileType;
	curDir->subFCB[tempSubFileFcbId].upperBlockId = currentBlockId;
	curDir->subFCB[tempSubFileFcbId].selfBlockId = tempFatId;
	curDir->subFCB[tempSubFileFcbId].initTime();
	curDir->subFCB[tempSubFileFcbId].selfAttrib = ReadAndWrite;         //Ĭ������Ϊ�ɶ�д
	curDir->subFCB[tempSubFileFcbId].selfSize = 0;

	char* p = diskPtr->data[tempFatId - 3];
	memset(p, 4, BLOCKSIZE);											// Ctrl+D ��4��ʼ�� �Ա����鿴�Ĺ��ܵ��ж����� 
	return true;														//�ɹ����
}

//echo д�ļ�����
bool echo(char* subFileName)
{
	loadFromDisk();														//���ش���
	Ddirectory* curDir = getCurrentDir();
	int tempSubFileId = findSameSubFileName(curDir, subFileName);
	if (tempSubFileId == 0)												//û�ҵ����ļ�������
	{
		color(4);
		cout << "����ʧ�ܣ��ļ������ڣ�" << endl;
		cout << "��ʾ�����ȴ����ļ����ٽ���д������" << endl;
		color(16);
		return false; //ʧ�ܣ�
	}
	char* bgn = diskPtr->data[curDir->subFCB[tempSubFileId].selfBlockId - 3];
	char* end = diskPtr->data[curDir->subFCB[tempSubFileId].selfBlockId - 2];

	color(4);
	cout << "��ʾ�����������ݣ���Ctrl + D��ϼ���������" << endl;
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
	}*bgn = 4;															//��Ϊ������־
	saveToDisk();														//ͬ������
	return true;
}

//more �鿴�ļ�����
bool more(char* subFileName)
{
	loadFromDisk();																	//����
	Ddirectory* curDir = getCurrentDir();											//��ǰĿ¼
	int tempSubFileId = findSameSubFileName(curDir, subFileName);
	if (tempSubFileId == 0)															//û�ҵ����ļ�������
	{
		color(4);
		cout << "����ʧ�ܣ��ļ������ڣ�" << endl;
		cout << "��ʾ�����ȴ����ļ����ٲ鿴���ݡ�" << endl;
		color(16);
		return false; //ʧ�ܣ�
	}
	char* bgn = diskPtr->data[curDir->subFCB[tempSubFileId].selfBlockId - 3];
	char *end = diskPtr->data[curDir->subFCB[tempSubFileId].selfBlockId - 2];
	color(4);
	cout << "��ʾ���ļ��������¡�" << endl;
	color(16);
	while (bgn < end && (*bgn) != 4) cout << *(bgn++);
	cout << endl;
	saveToDisk();																	//ͬ��
}

//find �ڵ�ǰĿ¼��Ѱ����ƥ��ָ���ַ������ļ�
bool find(char* str)
{
	loadFromDisk();
	Ddirectory *curDir = getCurrentDir();		//��ǰĿ¼��ָ��
	bool flag = false;
	for (int i = 1; i < DIRSIZE; i++)
	{
		if (curDir->subFCB[i].selfType == OFileType)
		{
			//�����ļ�������ݣ�����Ҫ����ע�� 
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
		cout << "��ʾ��δ�ҵ������ַ���" << str << "���ı��ļ���" << endl;
		color(16);
		return false;
	}
	saveToDisk();
}

//del ɾ����ͨ�ļ�
bool del(char* delsubFileName)
{
	loadFromDisk();                       						 	//����
	Ddirectory* curDir = getCurrentDir();   						//��ǰĿ¼��ָ��
	int delSubFileId = findSameSubFileName(curDir, delsubFileName);
	if (delSubFileId)												//�������ͬ���ֵ��ļ��Ϳ���ɾ��
	{
		int delFatId = curDir->subFCB[delSubFileId].selfBlockId;	//Ҫɾ�����ļ����ڵ��̿��
		diskPtr->FAT[delFatId] = 0;									//���㣬Ϊ�հ�
		char *temp = diskPtr->data[delFatId - 3];					//��ʽ��
		memset(temp, 0, BLOCKSIZE);
		curDir->subFCB[delSubFileId].initial();
		saveToDisk();												//����������   CHECK:ע�⣬ͬ�������̵�ʱ����ҪŪ���� 
		return true;												//ɾ�����
	}
	else {
		color(4);
		cout << "����ɾ��ʧ�ܣ����ļ������ڡ�" << endl;
		cout << "��ʾ����˶��ļ������ٽ���ɾ������ʹ��dir����鿴��ǰĿ¼�µ��ļ���" << endl;
		color(16);
		return false;								//ɾ��ʧ��
	}
}

//
bool attrib()
{
	loadFromDisk();											//�ؼ���
	Ddirectory* curDir = getCurrentDir();					//�õ���ǰĿ¼��ָ��
	if (curDir->getSubNum())								//������ļ�
	{
		cout.setf(ios::left);								//������� 
		cout.width(14);
		cout << "�ļ�����";
		cout.width(14);
		cout << "�ļ���" << endl;
		for (int i = 1; i < DIRSIZE; i++)
		{
			if (curDir->subFCB[i].selfType != Neither)
			{
				cout.width(14);
				if (curDir->subFCB[i].selfAttrib == ReadOnly)
					cout << "ֻ��";
				else if (curDir->subFCB[i].selfAttrib == WriteOnly)
					cout << "ֻд";
				else if (curDir->subFCB[i].selfAttrib == ReadAndWrite)
					cout << "��д";
				cout << currentPath << curDir->subFCB[i].selfName << endl;
			}
		}
	}
	saveToDisk();											//ͬ�� 
}

//dir list �鿴��ǰĿ¼�µ��ļ���
bool dir()
{
	loadFromDisk();								//�ؼ���
	Ddirectory* curDir = getCurrentDir();  		//�õ���ǰĿ¼��ָ��
	if (curDir->getSubNum())					//������ļ�
	{
		cout.setf(ios::left);					//������� 
		cout.width(14);
		cout << "�ļ���";
		cout.width(14);
		cout << "�ļ�����";
		cout.width(14);
		cout << "�ļ�����";
		cout.width(14);
		cout << "�ļ���С";
		cout.width(14);
		cout << "����ʱ��" << endl;
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
					cout << "ֻ��";
				else if (curDir->subFCB[i].selfAttrib == WriteOnly)
					cout << "ֻд";
				else if (curDir->subFCB[i].selfAttrib == ReadAndWrite)
					cout << "��д";
				cout.width(14);
				if(curDir->subFCB[i].selfType == OFileType)
					cout << curDir->subFCB[i].getSize();
				else cout << "";
				curDir->subFCB[i].printTime();
			}			
		}
	}
	
	cout << "��ǰĿ¼�¹���" << curDir->getSubNum() << "��" << endl;
	cout << "              " << curDir->getSubFileNum() << "���ļ�" << endl;
	cout << "              " << curDir->getSubDirNum() << "��Ŀ¼�ļ�" << endl;
	saveToDisk();						//ͬ��
}

//cd �ı乤��·��������ָ����·����
bool cd(char *name)
{
	loadFromDisk();										//�������
	Ddirectory* curDir = getCurrentDir();				//��ʼ��
	int toCurBlockId = currentBlockId;
	string toAbsPath = currentPath;
	parsePath(name, curDir, toCurBlockId, toAbsPath);	//����ԭ����·���������õ�����Ŀ¼ָ�룬���ڵ��̿�ţ��Լ�����·��
	currentBlockId = toCurBlockId;						//��ǰ���̿���滻Ϊָ��Ŀ¼���ڵ��̿��
	currentPath = toAbsPath;							//��ǰ�ľ���·���滻Ϊָ��Ŀ¼�ľ���·��
}

//copy ����ǰĿ¼��ָ�����ļ����Ƶ�ָ��·����
bool copy(char *subFileName, char *toPath)
{
	loadFromDisk();																			//����
	Ddirectory* curDir = getCurrentDir();
	Ddirectory* toDir = getCurrentDir();
	int tempSubFileId = findSameSubFileName(curDir, subFileName);
	if (tempSubFileId == 0)																	//û�ҵ����ļ�������
	{
		color(4);
		cout << "����ʧ�ܣ��ļ������ڣ�" << endl;
		cout << "��ʾ�����ȴ����ļ����ٸ��ơ�" << endl;
		color(16);
		return false; 																		//ʧ�ܣ�
	}
	int temp1 = 0;																			//���պ���������ֵ
	string temp2 = "";
	parsePath(toPath, toDir, temp1, temp2);													//����to��Ŀ¼�£�����һ���ļ���������д��ȥ
	toDir = getCurrentDir(temp1);
	int tempSubFileFcbId = 0, tempFatId = 0;												//���պ���������ֵ
	if(!touch(subFileName, toDir, tempSubFileFcbId, tempFatId))	return false;				//����һ�����ļ� ע�⣺���ܻ�ʧ�ܣ�ʧ����Ҫ�˳�������ע�⣡ 
	curDir = getCurrentDir();																//����������һ��Ҫ�ص���ǰ�ļ��У����� 
	string content = diskPtr->data[curDir->subFCB[tempSubFileId].selfBlockId - 3];			//�����ļ������� 
	toDir->subFCB[tempSubFileFcbId].selfSize = content.length();						    //���ٿռ� 
	strcpy(diskPtr->data[tempFatId - 3], content.c_str());									//���ļ������ݸ��ƽ�ȥ 

	saveToDisk();
}

//cpdir ����ǰ·���µĿ�Ŀ¼���Ƶ�ָ��·����
bool cpdir(char *subDirName, char *toPath)
{
	loadFromDisk();																			//����
	Ddirectory* curDir = getCurrentDir();
	int tempSubDirId = findSameSubDirName(curDir, subDirName);
	if (tempSubDirId == 0)																	//û�ҵ����ļ�������
	{
		color(4);
		cout << "����ʧ�ܣ��ļ������ڣ�" << endl;
		cout << "��ʾ�����ȴ����ļ����ٸ��ơ�" << endl;
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

//move ����ǰĿ¼��ָ�����ļ��ƶ���ָ��·����
bool move(char *subFilename, char* toPath){
	loadFromDisk();																			//����
	copy(subFilename, toPath);																//�ȸ��ƹ�ȥ
	del(subFilename);																		//���Ƴɹ�����ɾ��ԭ�����ļ�
	saveToDisk();																			//����
	return true;
}

//rename Ϊ��ǰĿ¼�µ�ָ���ļ��޸�����
bool rename(char* oldName, char* newName)
{
	loadFromDisk();
	Ddirectory *curDir = getCurrentDir();
	int tempId = findSameSubFileName(curDir, oldName);										//��ͬ���ļ�
	if (tempId == 0)
		tempId = findSameSubDirName(curDir, oldName);										//��ͬ��Ŀ¼
	if (tempId == 0)																		//�����û�У�����false
	{
		color(4);
		cout << "��ǰĿ¼��û����Ϊ" << oldName << "���ļ���Ŀ¼" << endl;
		color(16);
		return false;
	}
	else {
		strcpy(curDir->subFCB[tempId].selfName, newName);									//����ҵ��˾�������
	}
	saveToDisk();
	return true;
}

//�ӱ��ش��������ļ��������뵽��������
bool importFormDisk(char *realDiskFile, char *virtualDiskFilePath)
{
	loadFromDisk();																			//����
	FILE* f;
	if ((f = fopen(realDiskFile, "r")) == NULL)
	{
		color(4);
		cout << "��ʾ�����ش��̵��ļ������ڣ�" << endl;
		color(16);
		return false;
	}
	//������ش��̵��ļ����ڣ��ȶ����ݣ�����content����
	char content[BLOCKSIZE],ch;
	memset(content, 4, BLOCKSIZE);
	int i = 0;
	while ((ch = getc(f)) != EOF) {
		content[i++] = ch;
		if (i >= BLOCKSIZE) {
			color(4);
			cout << "�����ļ������޷����롣" << endl;
			color(16);
			return false;
		}
	}content[i] = 4;

	Ddirectory* toDir = getCurrentDir();													//Ҫȥ����Ŀ¼���Ե�ǰĿ¼��ʼ��
	int temp1 = 0;																			//���պ���������ֵ
	string temp2 = "";
	parsePath(virtualDiskFilePath, toDir, temp1, temp2);									//����to��Ŀ¼�£�����һ���ļ���������д��ȥ
	toDir = getCurrentDir(temp1);
	int tempSubFileFcbId = 0, tempFatId = 0;												//���պ���������ֵ
	if (!touch(parseFileName(realDiskFile), toDir, tempSubFileFcbId, tempFatId))
		return false;																		//����һ�����ļ�~   CHECK�����ܻ�ʧ�ܣ�ʧ����Ҫ�˳�������ע�⣡ 
	toDir->subFCB[tempSubFileFcbId].selfSize = strlen(content);								//���ٿռ� 
	strcpy(diskPtr->data[tempFatId - 3], content);											//���ļ������ݸ��ƽ�ȥ 
	saveToDisk();																			//����
	return true;
}

//�ӵ�ǰĿ¼�µ����ļ������ش���
bool exportToDisk(char *virtualDiskFile, char *realDiskFilePath)
{
	loadFromDisk();																			//����
	Ddirectory *curDir = getCurrentDir();													//��ǰĿ¼
	//��鵱ǰĿ¼���Ƿ��������ļ�
	int tempSubFileId = findSameSubFileName(curDir, virtualDiskFile);
	if (!tempSubFileId) {
		color(4);
		cout << "���󣺵�ǰĿ¼�²�������Ϊ" << virtualDiskFile << "���ļ����޷�������" << endl;
		cout << "��ʾ����˶��ļ������ٵ�����" << endl;
		color(16);
		return false;
	}
	//������ڵĻ�
	FILE* f;
	string temp1 = realDiskFilePath, temp2 = virtualDiskFile;
	if ((f = fopen((temp1 + '\\' + temp2).c_str(), "w")) == NULL)							//windows����\���ָ�
	{
		color(4);
		cout << "�����޷����������ش��̵�ָ��Ŀ¼�£�����·����" << endl;
		color(16);
		return false;
	}
	int tempFileBlockId = curDir->subFCB[tempSubFileId].selfBlockId;
	//�����ļ�����
	char *content = new char[strlen(diskPtr->data[tempFileBlockId - 3]) + 1];
	strcpy(content, diskPtr->data[tempFileBlockId - 3]);
	while (*(content) != 4) {
		fputc(*content++, f);
	}fclose(f);
	saveToDisk();																			//����
	return true;
}

//xcopy ����ǰ·���µ�Ŀ¼�����Ƶ�ָ��·����
bool xcopy(char* dirName, char* toPath)
{
	Ddirectory *curDir = getCurrentDir();							//��ǰĿ¼
	if (curDir->getSubNum() > 0) {									//������ǿ�Ŀ¼��Ҫһ��һ���ĸ��ƹ�ȥ
		cpdir(dirName, toPath);
		Ddirectory* parseToDir;
		int toCurBlockId = 0;
		string toAbsPath = ""; 
		parsePath(toPath, parseToDir, toCurBlockId, toAbsPath);
		char* toPathch = new char[toAbsPath.length() + strlen(dirName) + 1 - 4];
		strcpy(toPathch, toAbsPath.substr(4).c_str());
		strcat(toPathch, dirName);
		cd(dirName);
		curDir = getCurrentDir();									//��ǰĿ¼
		int i = 1;
		for (; i < DIRSIZE; i++)
		{
			if (curDir->subFCB[i].selfType == OFileType)
				copy(curDir->subFCB[i].selfName, toPathch);
			else if (curDir->subFCB[i].selfType == DirType)
				xcopy(curDir->subFCB[i].selfName, toPathch);		//�ݹ飡������ȣ�
		}
		if (i == DIRSIZE){											//������һ��Ŀ¼�ļ���Ҫ���˵���һ��
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

//help����ӡ������Ϣ
void helpPrint()
{
	cout << "------help------" << endl;
	cout << "dir                      �鿴��ǰĿ¼�µ��ļ�" << endl;
	cout << "mkdir   [Ŀ¼��]         �ڵ�ǰĿ¼�´���Ŀ¼�ļ� " << endl;
	cout << "rmdir   [Ŀ¼��]         �ڵ�ǰĿ¼��ɾ��Ŀ¼�ļ� " << endl;
	cout << "cd      [Ŀ¼��]         �����¼�Ŀ¼�򷵻��ϼ�Ŀ¼ " << endl;
	cout << "del     [�ļ���]         �ڵ�ǰĿ¼��ɾ����ͨ�ļ� " << endl;
	cout << "touch   [�ļ���]         �ڵ�ǰĿ¼�´����ı��ļ� " << endl;
	cout << "echo    [�ļ���]         �Ե�ǰĿ¼��ָ���ı��ļ���д������ " << endl;
	cout << "more    [�ļ���]         ��ʾ��ǰĿ¼��ָ���ı��ļ������� " << endl;
	cout << "rename  [�ļ���/Ŀ¼��]  Ϊ��ǰĿ¼�µ�ָ���ļ��޸�����" << endl;
	cout << "move    [�ļ���][·��]   ����ǰĿ¼��ָ�����ļ��ƶ���ָ��·����" << endl;
	cout << "copy    [�ļ���][·��]   ����ǰĿ¼��ָ�����ļ����Ƶ�ָ��·����" << endl;
	cout << "cpdir   [Ŀ¼��]         ����ǰ·���µĿ�Ŀ¼���Ƶ�ָ��·����" << endl;
	cout << "xcopy   [Ŀ¼��]         ����ǰ·���µ�Ŀ¼���Ƶ�ָ��·����" << endl;
	cout << "import  [·��][·��]     �ӱ��ش��������ļ�" << endl;
	cout << "export  [�ļ���][·��]   �ӵ�ǰĿ¼�µ����ļ������ش���" << endl;
	cout << "attrib                   ��ʾ����ǰĿ¼���ļ�������" << endl;					
	cout << "find    [�ַ���]         �ڵ�ǰĿ¼��Ѱ����ƥ��ָ���ַ������ļ�" << endl;
	cout << "clear                    �����Ļ" << endl;
	cout << "time                     ��ʾϵͳʱ��" << endl;
	cout << "exit                     �˳��ļ�ϵͳ" << endl;
}

//clear �����Ļ
void clear()
{
	system("cls");
}

//��ӡ��ǰʱ��
void timePrint()
{
	time_t timep;
	time(&timep);
	struct tm *p = localtime(&timep);
	cout << 1900 + p->tm_year << "/" <<  p->tm_mon + 1 << "/" << p->tm_mday << " ";
	cout << p->tm_hour << ":" << p->tm_min << ":" << p->tm_sec<<endl;
}

//�˳� 
bool exitSys()
{
	saveToDisk();													//���������ļ����˳����ͷ��ڴ�
	free(diskPtr);													//CHECK:��Ҫ�ظ��ͷ�filePtr�� 
	return true;
}

int main()
{
	char temp1[20], temp2[30];										//��������Ĳ���
	startAddress = (char *)malloc(DISKSIZE);						//��������ռ䲢�ҳ�ʼ��
	diskPtr = (Disk *)(startAddress);								//������̳�ʼ��
	//���������ļ�ϵͳ����Ϣ
	//������ܼ��أ�����Ҫ��ʽ����ָ���ǵ�һ�δ���VFS��ʱ��
	if (!loadFromDisk()){
		welcomeInterface();
		initialVFS();
	}cout << "��ʾ������help�ɲ鿴������Ϣ��" << endl;
	//�Ѿ����غ���,��ʼ���������
	while (true)
	{
		cout << ">" << currentPath;									//ϵͳ��ʾ�� + ��ǰ·��
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
			cout << "�����ʽ�������������롣" << endl;
			color(16);
		}
	}
	system("pause");
	return 0;
}
