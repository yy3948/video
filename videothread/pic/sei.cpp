#include <iomanip>
#include <stdio.h>
//#include <unistd.h>
#include <malloc.h>
#include<cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include<cstring>
#include<cmath>
//#include "h264Process.h"

// #define H264_SIZE 1*1024*1024   //读取1MB文件大小
#define H264_SIZE 2*1024*1024   //读取0.1MB文件大小，大概包含2-3帧图像
//#define USER_DATA_SIZE 512   //定义用户自定义数据大小
#define SEI_FIX_SIZE 24   

using namespace std;
namespace SEI{
    static unsigned char start_code[] = {0x00,0x00,0x01};  //起始码
    static unsigned char sei_uuid[] = { 0x54, 0x8f, 0x83, 0x97, 0xf3, 0x23, 0x97, 0x4b, 
                                        0xb7, 0xc7, 0x4f, 0x3a, 0xb5, 0x6e, 0x89, 0x52 };   //自定义标识码
    static int sei_uuid_size = sizeof(sei_uuid);    //自定义标识码长度
    static unsigned char nal_type = 0x06;   //NAL类型,0x06标识该NAL单元为SEI
    static unsigned char payload_type = 0x05;   //0x05标识当前SEI编码格式为标准格式
    static unsigned char sei_tail = 0x80;   //SEI结尾对齐码
} 

int user_total_size = 0;

extern "C"{
	
int FindSei(unsigned char* h264_buf, int cur_index,int buf_size){
    int cur_index_bak = cur_index;
	//cout<<"buf_size:"<<buf_size<<endl; 
    if(cur_index > buf_size-4) {
//    	cout<<"cur_index:"<<cur_index<<endl;
    	return 0;
	} 
    else if(h264_buf[cur_index]==0x00 && h264_buf[cur_index+1]==0x00 && h264_buf[cur_index+2]==0x01 && h264_buf[cur_index+3]==0x06){   
	    cur_index += 4;
	    //cout<<"go1"<<endl;
        while(cur_index < buf_size){
            if(h264_buf[cur_index]==0x00 && h264_buf[cur_index+1]==0x00 && h264_buf[cur_index+2]==0x01 && h264_buf[cur_index-1]==0x80){
            	break;
			}
            else{
            	++cur_index;
			}
        }
        // printf("cur_index:%d,cur_index_bak:%d,len is:%d\n",cur_index,cur_index_bak,cur_index-cur_index_bak);
        //cout<<"1"<<endl;
        cout<<"original_sei_length:"<<cur_index-cur_index_bak<<endl;
        return cur_index-cur_index_bak;   //如果找到SEI字段，返回SEI字段长度       
    }
    else{
    	return 0;  //如果没找到SEI字段，则返回0	
	} 
	
}

int IFramelocat(unsigned char* h264_buf, int cur_index,int buf_size){

    if(cur_index > buf_size-4){
    	return 0;
	}
    else if(h264_buf[cur_index]==0x00 && h264_buf[cur_index+1]==0x00 && h264_buf[cur_index+2]==0x01 && h264_buf[cur_index+3]==0x25 ){
    	//cout<<"iframe"<<endl;
    	return 1;
	}
	else if(h264_buf[cur_index]==0x00 && h264_buf[cur_index+1]==0x00 && h264_buf[cur_index+2]==0x01 && h264_buf[cur_index+3]==0x65 ){
		//cout<<"iframe"<<endl;
    	return 1;
	}  
    else{
    	//cout<<"noframeI"<<endl;
    	return 0;
	} 
}

int PFramelocat(unsigned char* h264_buf, int cur_index,int buf_size){
    if(cur_index > buf_size-4){
    	return 0;
	}
    else if(h264_buf[cur_index]==0x00 && h264_buf[cur_index+1]==0x00 && h264_buf[cur_index+2]==0x01 && h264_buf[cur_index+3]==0x21 ){
    	//cout<<"pframe"<<endl;
    	return 1;
	}
	else if(h264_buf[cur_index]==0x00 && h264_buf[cur_index+1]==0x00 && h264_buf[cur_index+2]==0x01 && h264_buf[cur_index+3]==0x61 ){
		//cout<<"pframe"<<endl;		
    	return 1;
	}       
    else{
    	//cout<<"noframeP"<<endl;
    	return 0;
	}
}

int H264DelSeiInsertSeiBeforeFrame(unsigned char* h264_buf,int buf_size,unsigned char* sei_buf,int sei_buf_size,FILE *tmp){
    for(int i=0; i<buf_size; i++){
  
        int len_sei = FindSei(h264_buf,i,buf_size);   //查找SEI字段位置，若当前位置为SEI开始字符，则返回SEI长度，否则返回0

        int Iframe = IFramelocat(h264_buf,i,buf_size);
		int Pframe = PFramelocat(h264_buf,i,buf_size);
//		cout<<Pframe<<endl; 
        if(len_sei == 0){         //SEI字段长度为0，未找到SEI字段，原始内容直接写入
            //cout<<"okokok"<<endl; 
            if(IFramelocat(h264_buf,i,buf_size))   //SEI字段插入I帧之前
            {
            	cout<<"iframe local at:"<<i<<endl;
            	fwrite(sei_buf, 1, sei_buf_size, tmp);
            	//cout<<"sei_bufl:"<<sei_buf<<endl;
			}     
            else if(PFramelocat(h264_buf,i,buf_size))    //SEI字段插入P帧之前
            {
            	cout<<"pframe local at:"<<i<<endl;
				fwrite(sei_buf, 1, sei_buf_size, tmp);
			}                
            fwrite(&h264_buf[i], 1, 1, tmp);    //将原始码流写入文件
        }
        else if(len_sei > 0){
        	//cout<<"len1:"<<len_sei<<endl;
            i = i+len_sei-1;    //找到SEI字段，跳过码流buffer中SEI字段长度，相当于删除码流中原始SEI字段
            // --i;    //判断程序处理完之后，当前下标已经为下一个单元的起始位，for循环还会再加一，因为在此需要减一
        }
        else{
        	
        	printf("error: findSei() return value min i!\n");
		}
    }
    //cout<<"the sei buf which user write:"<<sei_buf<<endl;
    return 1;
}

int FillSeiPacket(unsigned char* sei_buffer, int sei_size, unsigned char* user_data, int user_data_size)
{
    if(sei_size-user_data_size < 24){
    	return -1;
	}
	unsigned char* data = (unsigned char*)sei_buffer;
    unsigned char user_data_size_high = user_data_size >> 8;    // 取用户数据长度高8位
    unsigned char user_data_size_low = user_data_size & 0xFF;   // 取用户数据长度低8位
    memcpy(data, SEI::start_code, sizeof(SEI::start_code)); // 插入4字节NALU开始码
	data += sizeof(SEI::start_code);  
	*data++ = SEI::nal_type; // 插入1字节NAL类型识别码,data指针偏移
	*data++ = SEI::payload_type; // 插入1字节SEI字段编码格式识别码,data指针偏移
	memcpy(data, SEI::sei_uuid, SEI::sei_uuid_size); // 插入16字节UUID自定义识别码
	data += SEI::sei_uuid_size;
    *data++ = user_data_size_high;  // 插入1字节用户数据长度高8位
    *data++ = user_data_size_low;  // 插入1字节用户数据长度低8位
	memcpy(data, user_data, user_data_size);    // 插入n字节用户自定义数据
	//cout<<"user_data："<<user_data[2]<<endl; 
	data += user_data_size;    
	//cout<<"data:"<<data<<endl;
	//data[sei_size-1] = SEI::sei_tail; 
	//cout<<"data:"<<data<<endl; //自定义数组未能够成功导入uuid导致 
	//*sei_buffer = *data;
    sei_buffer[sei_size-1] = SEI::sei_tail;  // 插入1字节SEI封装包结尾对齐码
    //sei_buffer = data;
    //*sei_buffer = *data;
    //cout<<"sei_buffer:"<<sei_buffer<<endl;
//	cout<<"user_data_final:"<<data<<endl; 
//  cout<<"length without /0:"<<data-sei_buffer<<endl; 

    if(data-sei_buffer>sei_size){
    	return -2;
	} 
	else{
		return 1;
	} 
}

int H264GetSeiUserdata(unsigned char* h264_buf, int h264_buf_size, unsigned char* user_buf, int user_buf_size){
    int sei_num=0,not_match_num=0;
    unsigned char* user_buf_point = user_buf;
    for(int i=0; i<h264_buf_size; i++){
        int len_sei = FindSei(h264_buf,i,h264_buf_size);    //获取码流中当前位置SEI字段长度
        //cout<<"user sei:"<<len_sei<<endl; 
        if(len_sei>0){
            ++sei_num;
            int not_match_flag=0;
            for(int j=0; j<16; j++){    //判断SEI字段中UUID是否匹配
                if(h264_buf[i+5+j] == SEI::sei_uuid[j]){
                	continue;
				}   
                else{
                    ++not_match_flag;
                    cout<<"no match"<<endl; 
                    break;
                }
            }
            if(not_match_flag){  //如果not_match_flag为1，则该SEI字段不是我们定义的
                not_match_num += not_match_flag;
				cout<<"no match flag"<<endl; }
            else{   //否则该SEI是我们自定义的
                int h264_user_data_len = (h264_buf[i+21]<<8) + (h264_buf[i+22]&0xFF);
                //cout<<count<<endl;
                printf("h264_buf[i+21]:%d,h264_buf[i+22]:%d,len_sei:%d\n",h264_buf[i+21],h264_buf[i+22],len_sei);
                printf("h264_user_data_len is %d\n",h264_user_data_len);
                if(h264_user_data_len!=(len_sei-24)){    //如果读取到的用户数据长度与实际测到的数据长度不匹配
                    printf("error:User data contains a stream header!\n");
                    printf("h264_buf[i+22]:%d,h264_buf[i+23]:%d,len_sei:%d\n",h264_buf[i+21],h264_buf[i+22],len_sei);
                    printf("h264_user_data_len is %d\n",h264_user_data_len);
                }
                else{   //若匹配，则正常提取用户数据
                    unsigned char* h264_usr_data = h264_buf+i+23;
                    if(h264_user_data_len > (user_buf_size+user_buf-user_buf_point)){ //判定当前写入的用户数据长度是否大于接收BUF的长度
                        printf("error:recv user_buf_size is too min! \n");
                        break;
                    }
                    memcpy(user_buf_point,h264_usr_data,sizeof(unsigned char)*h264_user_data_len);//将解析出的用户数据复制到接收BUF中
                    user_buf_point += h264_user_data_len;    //接收BUF指针偏移，方便下一帧数据写入
                    for(int k=0;k<10;k++)   //用户数据后面添加10个0作为用户数据分割标识
                       *user_buf_point++ = 'a';
                }
            }
            i+=len_sei-1;
        }
        else 
            continue;
    }
    // printf("not_match_num is %d\n",not_match_num);
    //cout<<sei_num<<endl;
    return sei_num;
   }


//SEI字段插入H264码流文件程序
__declspec(dllexport)
int insertSeiProcess() {
	
	ifstream fin("G:/pycharm/videothread1/data/verify_data.csv"); //打开文件流操作
	string line; 
	int count = 0;
	vector<string> fields;	//声明一个字符串向量 
	
	while (getline(fin, line))   //整行读取，换行符“\n”区分，遇到文件尾标志eof终止读取
	{
	 
		istringstream sin(line); //将整行字符串line读入到字符串流istringstream中	
		vector<string> sig;
		string field;
		
		while (getline(sin, field)) //还需再修改  将字符串流sin中的字符读入到field字符串中，以逗号为分隔符
		{
			if(count > 0){
			  fields.push_back(field); //将刚刚读取的字符串添加到向量fields中
			  }
			count=count+1;
		}
		
	}
	cout<<"signature number:"<<fields.size()<<endl;
	int number = fields.size();
	std::vector<std::string> arr;
	for(int i=0; i<number; i++){
		arr.push_back(fields[i]);
	}
	//cout<<arr[0]<<endl;
	//读取H264源文件，删去SEI字段
	FILE *h264_file=NULL;
	FILE *tmp = NULL;
	static unsigned char fullBuffer[H264_SIZE+4] = {0};
	unsigned char* buffer = fullBuffer;
	h264_file = fopen("G:/pycharm/videothread1/sei/sei.h264", "rb+");  //带SEI字段的H264源文件
	cout<<"original h264 files:"<<h264_file<<endl; 
	tmp = fopen("G:/pycharm/videothread1/sei/tmp.h264","wb+");  //写入自定义数据的的新H264文件
	if (!h264_file) {
		printf("ERROR:Open h264 file fialed.\n");
		return -1;
	}
	int size = fread(buffer, 1, H264_SIZE, h264_file);
	printf("h264 size:%d\n",size,"bytes");
	
	for(int i=0; i<number; i++){
		int user_data_size = arr[i].length();
		user_total_size = user_total_size + user_data_size + 10;
		int sei_size = user_data_size + SEI_FIX_SIZE;
		cout<<"user_sei_size:"<<sei_size<<endl;
		unsigned char** user_data_buf = (unsigned char**)malloc(sizeof(unsigned char*)*user_data_size);//分配内存 
		unsigned char* sei_buf = (unsigned char*)malloc(sizeof(unsigned char)*sei_size);
		//cout<<sizeof(unsigned char)*user_data_size<<endl;
		if(user_data_buf==NULL || sei_buf==NULL) {
			printf("malloc faild!\n");
			return -1;
		}
	
		memset(user_data_buf,0,sizeof(unsigned char)*user_data_size);//填充对应内存 
		memset(sei_buf,0,sizeof(unsigned char)*sei_size);
		user_data_buf[i]=(unsigned char*)arr[i].c_str();
		int sei_stat = FillSeiPacket(sei_buf, sei_size, user_data_buf[i], user_data_size);   //将用户自定义数据封装成SEI格式包
		cout<<"user_data_buf:"<<user_data_buf[i]<<endl; 
	    if(sei_stat < 0) printf("fillSeiPacket faild\n");
	    else printf("fillSeiPacket successed\n");

		H264DelSeiInsertSeiBeforeFrame(buffer,size,sei_buf,sei_size,tmp);
		free(user_data_buf);    //释放动态内存空间
		free(sei_buf);
		//cout<<(unsigned char*)arr[i].c_str()<<endl;
	}

	//cout<<"user_data_buf:"<<user_data_buf<<endl;
	//cout<<"user_data_buf[0]:"<<user_data_buf[1]<<endl;
	//sei_buf = (unsigned char*)"abcde";
	
	cout<<"----------------write end-----------------"<<endl;

	fclose(h264_file);
	fclose(tmp);
}

//从H264文件中提取自定义SEI字段
__declspec(dllexport)
int getSeiProcess() {
	FILE* h264_file = fopen("G:/pycharm/videothread1/sei/tmp.h264","rb+");	//码流源文件
	FILE* sei_file = fopen("G:/pycharm/videothread1/sei/tmp.txt","w+");	//将读取到的自定义数据写入txt文档
	FILE* tmp1_file = fopen("G:/pycharm/videothread1/sei/h264.txt","w+");	//将H264码流数据读出来以便debug
	if (!h264_file || !sei_file) {
		printf("ERROR:Open tmp.h264 or tmp.txt fialed.\n");
		return -1;
	}
	fseek(h264_file, 0, SEEK_END);      //将文件指针偏移到文件尾
	int file_size = ftell(h264_file);   //获取h264文件大小
	fseek(h264_file, 0, SEEK_SET);      //将文件指针偏移到文件头
	unsigned char* h264_buf = (unsigned char*)malloc(sizeof(unsigned char)*file_size);  //开辟h264_buf空间
	memset(h264_buf,0,sizeof(unsigned char)*file_size);     //初始化h264_buf
	int h264_size = fread(h264_buf, 1, file_size, h264_file);   //将h264文件读到h264_buf中
	printf("h264 file size is %d\n",h264_size);
	
	ifstream fin("G:/pycharm/videothread1/data/verify_data.csv"); //打开文件流操作
	string line; 
	int count = 0;
	vector<string> fields;	//声明一个字符串向量 
	while (getline(fin, line))   //整行读取，换行符“\n”区分，遇到文件尾标志eof终止读取
	{
		istringstream sin(line); //将整行字符串line读入到字符串流istringstream中	
		vector<string> sig;
		string field;
		while (getline(sin, field)) //还需再修改  将字符串流sin中的字符读入到field字符串中，以逗号为分隔符
		{
			if(count > 0){
			  fields.push_back(field); //将刚刚读取的字符串添加到向量fields中
			  }
			count=count+1;
		}
	}
	int number = fields.size();
	std::vector<std::string> arr;
	for(int i=0; i<number; i++){
		arr.push_back(fields[i]);
	}
	for(int i=0; i<number; i++){
		int user_data_size = arr[i].length();
		user_total_size = user_total_size + user_data_size + 10;}

	unsigned char* user_buf = (unsigned char*)malloc(sizeof(unsigned char)*user_total_size);  //开辟SEI接收B空间
	memset(user_buf,0,sizeof(unsigned char)*user_total_size);     //初始化h264_buf
	int find_sei_num = H264GetSeiUserdata(h264_buf, h264_size, user_buf, user_total_size);
	printf("found sei num is %d\n",find_sei_num);
	for(int i=0; i<h264_size; i++){
		fprintf(tmp1_file, "%x", h264_buf[i]);
	}
		
	for(int i=0; i<user_total_size; i++){
		fprintf(sei_file, "%c", user_buf[i]);
	}
	
	cout<<"----------------read end-----------------"<<endl;	

	//printf("\n");
	free(h264_buf);
	free(user_buf);
	fclose(h264_file);
	fclose(sei_file);
	fclose(tmp1_file);
}

int main() {
	
	insertSeiProcess();
	getSeiProcess();

	return 0;
  }
}
