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

// #define H264_SIZE 1*1024*1024   //��ȡ1MB�ļ���С
#define H264_SIZE 2*1024*1024   //��ȡ0.1MB�ļ���С����Ű���2-3֡ͼ��
//#define USER_DATA_SIZE 512   //�����û��Զ������ݴ�С
#define SEI_FIX_SIZE 24   

using namespace std;
namespace SEI{
    static unsigned char start_code[] = {0x00,0x00,0x01};  //��ʼ��
    static unsigned char sei_uuid[] = { 0x54, 0x8f, 0x83, 0x97, 0xf3, 0x23, 0x97, 0x4b, 
                                        0xb7, 0xc7, 0x4f, 0x3a, 0xb5, 0x6e, 0x89, 0x52 };   //�Զ����ʶ��
    static int sei_uuid_size = sizeof(sei_uuid);    //�Զ����ʶ�볤��
    static unsigned char nal_type = 0x06;   //NAL����,0x06��ʶ��NAL��ԪΪSEI
    static unsigned char payload_type = 0x05;   //0x05��ʶ��ǰSEI�����ʽΪ��׼��ʽ
    static unsigned char sei_tail = 0x80;   //SEI��β������
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
        return cur_index-cur_index_bak;   //����ҵ�SEI�ֶΣ�����SEI�ֶγ���       
    }
    else{
    	return 0;  //���û�ҵ�SEI�ֶΣ��򷵻�0	
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
  
        int len_sei = FindSei(h264_buf,i,buf_size);   //����SEI�ֶ�λ�ã�����ǰλ��ΪSEI��ʼ�ַ����򷵻�SEI���ȣ����򷵻�0

        int Iframe = IFramelocat(h264_buf,i,buf_size);
		int Pframe = PFramelocat(h264_buf,i,buf_size);
//		cout<<Pframe<<endl; 
        if(len_sei == 0){         //SEI�ֶγ���Ϊ0��δ�ҵ�SEI�ֶΣ�ԭʼ����ֱ��д��
            //cout<<"okokok"<<endl; 
            if(IFramelocat(h264_buf,i,buf_size))   //SEI�ֶβ���I֮֡ǰ
            {
            	cout<<"iframe local at:"<<i<<endl;
            	fwrite(sei_buf, 1, sei_buf_size, tmp);
            	//cout<<"sei_bufl:"<<sei_buf<<endl;
			}     
            else if(PFramelocat(h264_buf,i,buf_size))    //SEI�ֶβ���P֮֡ǰ
            {
            	cout<<"pframe local at:"<<i<<endl;
				fwrite(sei_buf, 1, sei_buf_size, tmp);
			}                
            fwrite(&h264_buf[i], 1, 1, tmp);    //��ԭʼ����д���ļ�
        }
        else if(len_sei > 0){
        	//cout<<"len1:"<<len_sei<<endl;
            i = i+len_sei-1;    //�ҵ�SEI�ֶΣ���������buffer��SEI�ֶγ��ȣ��൱��ɾ��������ԭʼSEI�ֶ�
            // --i;    //�жϳ�������֮�󣬵�ǰ�±��Ѿ�Ϊ��һ����Ԫ����ʼλ��forѭ�������ټ�һ����Ϊ�ڴ���Ҫ��һ
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
    unsigned char user_data_size_high = user_data_size >> 8;    // ȡ�û����ݳ��ȸ�8λ
    unsigned char user_data_size_low = user_data_size & 0xFF;   // ȡ�û����ݳ��ȵ�8λ
    memcpy(data, SEI::start_code, sizeof(SEI::start_code)); // ����4�ֽ�NALU��ʼ��
	data += sizeof(SEI::start_code);  
	*data++ = SEI::nal_type; // ����1�ֽ�NAL����ʶ����,dataָ��ƫ��
	*data++ = SEI::payload_type; // ����1�ֽ�SEI�ֶα����ʽʶ����,dataָ��ƫ��
	memcpy(data, SEI::sei_uuid, SEI::sei_uuid_size); // ����16�ֽ�UUID�Զ���ʶ����
	data += SEI::sei_uuid_size;
    *data++ = user_data_size_high;  // ����1�ֽ��û����ݳ��ȸ�8λ
    *data++ = user_data_size_low;  // ����1�ֽ��û����ݳ��ȵ�8λ
	memcpy(data, user_data, user_data_size);    // ����n�ֽ��û��Զ�������
	//cout<<"user_data��"<<user_data[2]<<endl; 
	data += user_data_size;    
	//cout<<"data:"<<data<<endl;
	//data[sei_size-1] = SEI::sei_tail; 
	//cout<<"data:"<<data<<endl; //�Զ�������δ�ܹ��ɹ�����uuid���� 
	//*sei_buffer = *data;
    sei_buffer[sei_size-1] = SEI::sei_tail;  // ����1�ֽ�SEI��װ����β������
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
        int len_sei = FindSei(h264_buf,i,h264_buf_size);    //��ȡ�����е�ǰλ��SEI�ֶγ���
        //cout<<"user sei:"<<len_sei<<endl; 
        if(len_sei>0){
            ++sei_num;
            int not_match_flag=0;
            for(int j=0; j<16; j++){    //�ж�SEI�ֶ���UUID�Ƿ�ƥ��
                if(h264_buf[i+5+j] == SEI::sei_uuid[j]){
                	continue;
				}   
                else{
                    ++not_match_flag;
                    cout<<"no match"<<endl; 
                    break;
                }
            }
            if(not_match_flag){  //���not_match_flagΪ1�����SEI�ֶβ������Ƕ����
                not_match_num += not_match_flag;
				cout<<"no match flag"<<endl; }
            else{   //�����SEI�������Զ����
                int h264_user_data_len = (h264_buf[i+21]<<8) + (h264_buf[i+22]&0xFF);
                //cout<<count<<endl;
                printf("h264_buf[i+21]:%d,h264_buf[i+22]:%d,len_sei:%d\n",h264_buf[i+21],h264_buf[i+22],len_sei);
                printf("h264_user_data_len is %d\n",h264_user_data_len);
                if(h264_user_data_len!=(len_sei-24)){    //�����ȡ�����û����ݳ�����ʵ�ʲ⵽�����ݳ��Ȳ�ƥ��
                    printf("error:User data contains a stream header!\n");
                    printf("h264_buf[i+22]:%d,h264_buf[i+23]:%d,len_sei:%d\n",h264_buf[i+21],h264_buf[i+22],len_sei);
                    printf("h264_user_data_len is %d\n",h264_user_data_len);
                }
                else{   //��ƥ�䣬��������ȡ�û�����
                    unsigned char* h264_usr_data = h264_buf+i+23;
                    if(h264_user_data_len > (user_buf_size+user_buf-user_buf_point)){ //�ж���ǰд����û����ݳ����Ƿ���ڽ���BUF�ĳ���
                        printf("error:recv user_buf_size is too min! \n");
                        break;
                    }
                    memcpy(user_buf_point,h264_usr_data,sizeof(unsigned char)*h264_user_data_len);//�����������û����ݸ��Ƶ�����BUF��
                    user_buf_point += h264_user_data_len;    //����BUFָ��ƫ�ƣ�������һ֡����д��
                    for(int k=0;k<10;k++)   //�û����ݺ������10��0��Ϊ�û����ݷָ��ʶ
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


//SEI�ֶβ���H264�����ļ�����
__declspec(dllexport)
int insertSeiProcess() {
	
	ifstream fin("G:/pycharm/videothread1/data/verify_data.csv"); //���ļ�������
	string line; 
	int count = 0;
	vector<string> fields;	//����һ���ַ������� 
	
	while (getline(fin, line))   //���ж�ȡ�����з���\n�����֣������ļ�β��־eof��ֹ��ȡ
	{
	 
		istringstream sin(line); //�������ַ���line���뵽�ַ�����istringstream��	
		vector<string> sig;
		string field;
		
		while (getline(sin, field)) //�������޸�  ���ַ�����sin�е��ַ����뵽field�ַ����У��Զ���Ϊ�ָ���
		{
			if(count > 0){
			  fields.push_back(field); //���ոն�ȡ���ַ�����ӵ�����fields��
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
	//��ȡH264Դ�ļ���ɾȥSEI�ֶ�
	FILE *h264_file=NULL;
	FILE *tmp = NULL;
	static unsigned char fullBuffer[H264_SIZE+4] = {0};
	unsigned char* buffer = fullBuffer;
	h264_file = fopen("G:/pycharm/videothread1/sei/sei.h264", "rb+");  //��SEI�ֶε�H264Դ�ļ�
	cout<<"original h264 files:"<<h264_file<<endl; 
	tmp = fopen("G:/pycharm/videothread1/sei/tmp.h264","wb+");  //д���Զ������ݵĵ���H264�ļ�
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
		unsigned char** user_data_buf = (unsigned char**)malloc(sizeof(unsigned char*)*user_data_size);//�����ڴ� 
		unsigned char* sei_buf = (unsigned char*)malloc(sizeof(unsigned char)*sei_size);
		//cout<<sizeof(unsigned char)*user_data_size<<endl;
		if(user_data_buf==NULL || sei_buf==NULL) {
			printf("malloc faild!\n");
			return -1;
		}
	
		memset(user_data_buf,0,sizeof(unsigned char)*user_data_size);//����Ӧ�ڴ� 
		memset(sei_buf,0,sizeof(unsigned char)*sei_size);
		user_data_buf[i]=(unsigned char*)arr[i].c_str();
		int sei_stat = FillSeiPacket(sei_buf, sei_size, user_data_buf[i], user_data_size);   //���û��Զ������ݷ�װ��SEI��ʽ��
		cout<<"user_data_buf:"<<user_data_buf[i]<<endl; 
	    if(sei_stat < 0) printf("fillSeiPacket faild\n");
	    else printf("fillSeiPacket successed\n");

		H264DelSeiInsertSeiBeforeFrame(buffer,size,sei_buf,sei_size,tmp);
		free(user_data_buf);    //�ͷŶ�̬�ڴ�ռ�
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

//��H264�ļ�����ȡ�Զ���SEI�ֶ�
__declspec(dllexport)
int getSeiProcess() {
	FILE* h264_file = fopen("G:/pycharm/videothread1/sei/tmp.h264","rb+");	//����Դ�ļ�
	FILE* sei_file = fopen("G:/pycharm/videothread1/sei/tmp.txt","w+");	//����ȡ�����Զ�������д��txt�ĵ�
	FILE* tmp1_file = fopen("G:/pycharm/videothread1/sei/h264.txt","w+");	//��H264�������ݶ������Ա�debug
	if (!h264_file || !sei_file) {
		printf("ERROR:Open tmp.h264 or tmp.txt fialed.\n");
		return -1;
	}
	fseek(h264_file, 0, SEEK_END);      //���ļ�ָ��ƫ�Ƶ��ļ�β
	int file_size = ftell(h264_file);   //��ȡh264�ļ���С
	fseek(h264_file, 0, SEEK_SET);      //���ļ�ָ��ƫ�Ƶ��ļ�ͷ
	unsigned char* h264_buf = (unsigned char*)malloc(sizeof(unsigned char)*file_size);  //����h264_buf�ռ�
	memset(h264_buf,0,sizeof(unsigned char)*file_size);     //��ʼ��h264_buf
	int h264_size = fread(h264_buf, 1, file_size, h264_file);   //��h264�ļ�����h264_buf��
	printf("h264 file size is %d\n",h264_size);
	
	ifstream fin("G:/pycharm/videothread1/data/verify_data.csv"); //���ļ�������
	string line; 
	int count = 0;
	vector<string> fields;	//����һ���ַ������� 
	while (getline(fin, line))   //���ж�ȡ�����з���\n�����֣������ļ�β��־eof��ֹ��ȡ
	{
		istringstream sin(line); //�������ַ���line���뵽�ַ�����istringstream��	
		vector<string> sig;
		string field;
		while (getline(sin, field)) //�������޸�  ���ַ�����sin�е��ַ����뵽field�ַ����У��Զ���Ϊ�ָ���
		{
			if(count > 0){
			  fields.push_back(field); //���ոն�ȡ���ַ�����ӵ�����fields��
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

	unsigned char* user_buf = (unsigned char*)malloc(sizeof(unsigned char)*user_total_size);  //����SEI����B�ռ�
	memset(user_buf,0,sizeof(unsigned char)*user_total_size);     //��ʼ��h264_buf
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
