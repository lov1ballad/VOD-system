#include <iostream>
#include <mysql/mysql.h>
#include <jsoncpp/json/json.h>
#include <mutex>
#include <fstream>

//数据管理模块

namespace vod_system
{
#define MYSQL_HOST "127.0.0.1"
#define MYSQL_USER "root"
#define MYSQL_PASS "ljl12138"
#define MYSQL_NAME "vod_system"


    static MYSQL *MysqlInit()//初始化接口
    {
        MYSQL *mysql = mysql_init(NULL);//句柄初始化
        if(mysql == NULL)
        {
            std::cout<<"mysql init failed!\n";
            return NULL;
        }
        if(mysql_real_connect(mysql,MYSQL_HOST,MYSQL_USER,
                    MYSQL_PASS,MYSQL_NAME,0,NULL,0)==NULL)//连接服务器
        {
            std::cout<<mysql_error(mysql)<<std::endl;
            mysql_close(mysql);
            return NULL;
        }
        if(mysql_set_character_set(mysql,"utf8")!=0)
        {//设置字符集
            std::cout<<mysql_error(mysql)<<std::endl;
            mysql_close(mysql);
            return NULL;
        }
        return mysql;

    }
    static void MysqlRelease(MYSQL *mysql)//数据库释放接口
    {
        if(mysql != NULL)
        {
            mysql_close(mysql);
        }
        return;
    }
    static bool  MysqlQuery(MYSQL *mysql, const std::string sql)//执行语句操作接口
    {//执行接口封装
        int ret = mysql_query(mysql,sql.c_str());
        if(ret != 0)//执行失败
        {
         std::cout<<sql<<std::endl;//要执行的语句
         std::cout<<mysql_error(mysql)<<std::endl;
         return false;
        }
        return true;
    }
    /////////////////////////////////
    //数据库访问类
    class TableVod
    {
        private:
            MYSQL *_mysql;
            std::mutex _mutex;
        public:
            TableVod()//数据库句柄初始化连接服务器
            {
                _mysql = MysqlInit();
                if(_mysql == NULL)
                {
                    exit(0);
                }
            }
            ~TableVod()//数据库句柄销毁
            {
                MysqlRelease(_mysql);
            }

	    //增
            bool Insert(const Json::Value &video)
            {
               const char* name = video["name"].asCString();
               const char* vdesc = video["vdesc"].asCString();
               const char *video_url = video["video_url"].asCString();
               const char* image_url = video["image_url"].asCString();
                char sql[8192] = {0};
#define VIDEO_INSERT "insert tb_video values(null,'%s','%s','%s','%s',now());"
                sprintf(sql,VIDEO_INSERT, name,vdesc,video_url,image_url);
                return MysqlQuery(_mysql,sql);
            }

	    //删
            bool Delete(int video_id)
            {
#define VIDEO_DELETE "delete from tb_video where id=%d;"
                char sql[8192] = {0};
                sprintf(sql,VIDEO_DELETE,video_id);
                return MysqlQuery(_mysql,sql);
            }

	    //改
            bool Update(int video_id,const Json::Value &video)
            {
#define VIDEO_UPDATE "update tb_video set name='%s',vdesc='%s' where id=%d;"
                char sql[8192] = {0};
                sprintf(sql,VIDEO_UPDATE,video["name"].asCString(),
                        video["vdesc"].asCString(),
                        video_id);
                return MysqlQuery(_mysql,sql);
            }

	    //查
            bool GetAll(Json::Value *video)
            {
#define VIDEO_GETALL "select * from tb_video;"
                _mutex.lock();
                bool ret = MysqlQuery(_mysql,VIDEO_GETALL);//查询结果集
                if(ret ==false)
                {
                    _mutex.unlock();
                    return false;
                }
                MYSQL_RES * res = mysql_store_result(_mysql);//保存结果集
                _mutex.unlock();
                if(res ==NULL)
                {
                    std::cout<<"store result failed!\n";
                    return false;
                }
                int num = mysql_num_rows(res);//获取结果集条数
                for(int i = 0;i<num;++i)
                {
                    MYSQL_ROW row = mysql_fetch_row(res);//遍历结果集
                    Json::Value val;
                    val["id"] = std::stoi(row[0]);
                    val["name"] = row[1];
                    val["vdesc"] = row[2];
                    val["video_url"] = row[3];
                    val["image_url"] = row[4];
                    val["ctime"] = row[5];
                    video->append(val);//添加数组元素，每一条都是一个数组元素
                }
                mysql_free_result(res);//释放结果集
                return true;
            }
            bool GetOne(int video_id,Json::Value *video)
            {
#define VIDEO_GETONE "select * from tb_video where id=%d;"
                char sql_str[4096] = {0};
                sprintf(sql_str,VIDEO_GETONE,video_id);
                _mutex.lock();
                bool ret = MysqlQuery(_mysql,sql_str);
                if(ret == false)
                {
                    _mutex.unlock();
                    return false;
                }
                MYSQL_RES *res = mysql_store_result(_mysql);
                _mutex.unlock();
                if(res == NULL)
                {
                    std::cout<<mysql_error(_mysql)<<std::endl;
                    return false;
                }
                int num_row = mysql_num_rows(res);
                if(num_row != 1)
                {
                    std::cout<<"getone result error\n";
                    mysql_free_result(res);
                    return false;
                }
                MYSQL_ROW row = mysql_fetch_row(res);//从结果集获取一条结果
                (*video)["id"] = video_id;
                (*video)["name"] = row[1];
                (*video)["vdesc"] = row[2];
                (*video)["video_url"] = row[3];
                (*video)["image_url"] = row[4];
                (*video)["ctime"] = row[5];
                mysql_free_result(res);
                return true;
            }
    };
    class Util
    {
    public:
    	static bool WriteFile(const std::string &name,const std::string &content)
	{
		std::ofstream of;
		of.open(name,std::ios::binary);
		if(!of.is_open())
		{
			std::cout<<"open file failed!\n";
			return false;
		}
		of.write(content.c_str(),content.size());
		if(!of.good())
		{
			std::cout<<"write file failed!\n";
			return false;
		}
		of.close();
		return true;
	}
    };
}
