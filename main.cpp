#include <boost/algorithm/string.hpp>
#include "db.hpp"
#include "httplib.h"
#include <fstream>

#define WWWROOT "./wwwroot"

///video/**.mp4(数据库中的存储的的视频信息)

using namespace httplib;

vod_system::TableVod *tb_video;//why全局变量,因为httplib库是基于多线程



void VideoDelete(const Request &req, Response &rsp)
{
	//req.path = /video/1  req.matches存放正则表达式匹配到的内容
	//1.获取视频id
	int video_id= std::stoi(req.matches[1]);

	//2.从数据库中获取到对应视频信息
	Json::Value json_rsp;
	Json::FastWriter writer;
	Json::Value video;
	bool ret = tb_video->GetOne(video_id,&video);
	if(ret == false)
	{
		std::cout<< "mysql get video info failed!\n";
		rsp.status = 500;
		json_rsp["result"] = false;
		json_rsp["reason"] = "mysql get video info failed!";
		rsp.body = writer.write(json_rsp);
		rsp.set_header("Content-Type","application/json");
		return;
	}
	std::string vpath = WWWROOT + video["video_url"].asString();
	std::string ipath = WWWROOT + video["image_url"].asString();

	//3.删除视频文件，封面图文件
	//unlink删除指定名字的文件
	unlink(vpath.c_str());
	unlink(ipath.c_str());

	//4.删除数据库中的数据
	ret = tb_video->Delete(video_id);
	if(ret == false)
	{
		rsp.status = 500;
		std::cout<<"mysql delete video failed!\n";
		return;
	}
	return;
}

void VideoUpdate(const Request &req, Response &rsp)
{
    //1.获取视频id
	int video_id= std::stoi(req.matches[1]);
	Json::Value video;
	Json::Reader reader;
	bool ret = reader.parse(req.body ,video);//提交上来的需要修改的json序列化后的字符串
	if(ret ==false)
	{
		std::cout<<"update video : parse video json failed!\n";
		rsp.status = 400;//表示客户端错误
		return;
	}
	ret = tb_video->Update(video_id, video);
	if(ret == false)
	{
		std::cout<<"update video:mysql update failed!\n";
		rsp.status=5000;//服务器内部错误
		return;
	}
	return;
}

void VideoGetAll(const Request &req, Response &rsp)
{
    Json::Value videos;
	Json::FastWriter writer;
	bool ret = tb_video->GetAll(&videos);
	if(ret == false)
	{
		std::cout<<"getall video:mysql operation failed!\n";
		rsp.status = 500;
		return;
	}
	rsp.body = writer.write(videos);
	rsp.set_header("Content-Type", "application/json");
	
}

void VideoGetOne(const Request &req, Response &rsp)
{
    int video_id= std::stoi(req.matches[1]);
	Json::Value video;
	Json::FastWriter writer;
	bool ret = tb_video->GetOne(video_id,&video);
	if(ret == false)
	{
		std::cout<<"getone video:mysql operation failed!\n";
		rsp.status = 500;
		return;
	}
	rsp.body = writer.write(video);
	rsp.set_header("Content-Type", "application/json");
}

#define VIDEO_PATH "/video/"
#define IMAGE_PATH "/image/"
void VideoUpload(const Request &req, Response &rsp)
{
	//视频名称
    auto ret = req.has_file("video_name");//判断是否存在该字段
	if(ret == false)
	{
		std::cout<<"have no video name!\n";
		rsp.status = 400;//请求格式问题
		return;
	}
	const auto & file = req.get_file_value("video_name");//获取该字段对应的信息
	
	
	//视频描述
	ret = req.has_file("video_desc");
	if(ret == false)
	{
		std::cout<<"have no video desc!\n";
		rsp.status = 400;//请求格式问题
		return;
	}
	const auto& file1 = req.get_file_value("video_desc");
	

	//视频文件
	ret = req.has_file("video_file");
	if(ret == false)
	{
		std::cout<<"have no video file!\n";
		rsp.status = 400;//请求格式问题
		return;
	}
	const auto& file2 = req.get_file_value("video_file");
	

	//封面文件
	ret = req.has_file("image_file");
	if(ret == false)
	{
		std::cout<<"have no image file!\n";
		rsp.status = 400;//请求格式问题
		return;
	}
	const auto& file3 = req.get_file_value("image_file");
	const std::string &videoname = file.content;//视频名
	const std::string &videodesc = file1.content;//描述
	const std::string& videofile = file2.filename;//***.mp4
	const std::string& videocont = file2.content;//视频文件数据
	const std::string &imagefile = file3.filename;//***。jpg
	const std::string &imagecont = file3.content;//封面文件数据

	///组织路径
	std::string vurl = VIDEO_PATH + file2.filename;
	std::string iurl = IMAGE_PATH + file3.filename;
	std::string wwwroot = WWWROOT;//因为WWWROOT是const char *没有重载+
	vod_system::Util::WriteFile(wwwroot + vurl,file2.content);//组织出来的实际路径并完成文件写入
	
	vod_system::Util::WriteFile(wwwroot + iurl,file3.content);
	
	Json::Value video;
	video["name"] = videoname;
	video["vdesc"] = videodesc;
	video["video_url"] = vurl;
	video["image_url"] = iurl;

	ret = tb_video->Insert(video);
	if(ret == false)
	{
		rsp.status = 500;//服务器内部错误
		std::cout<<"insert video: mysql operation failed!\n";
		return;
	}
    rsp.set_redirect("/");
    return;

}

bool ReadFile(const std::string &name, std::string *body)
{
    std::ifstream ifile;
    ifile.open(name, std::ios::binary);
    if(!ifile.is_open())
    {
        printf("open file failed111:%s\n",name.c_str());
        ifile.close();
        return false;
    }
    ifile.seekg(0,std::ios::end);
    uint64_t length = ifile.tellg();
    ifile.seekg(0,std::ios::beg);
    body->resize(length);
    ifile.read(&(*body)[0],length);
    if(ifile.good() == false)
    {
        printf("read file failed:%s\n", name.c_str());
        ifile.close();
        return false;
    }
    ifile.close();
    return true;
}

void VideoPlay(const Request &req, Response &rsp)
{
    Json::Value video;
    int video_id = std::stoi(req.matches[1]);
	bool ret = tb_video->GetOne(video_id,&video);
	if(ret == false)
	{
		std::cout<<"getone video:mysql operation failed!\n";
		rsp.status = 500;
		return;
    }
    std::string newstr = video["video_url"].asString();
    std::string oldstr = "{{video_url}}";
    std::string play_html = "./wwwroot/single-video.html";
    ReadFile(play_html,&rsp.body);
    boost::algorithm::replace_all(rsp.body,oldstr,newstr);
    rsp.set_header("Content-Type", "text/html");
    return;

}

int main()
{
	tb_video = new vod_system::TableVod();

    Server srv;
	srv.set_base_dir("./wwwroot");
	///////动态数据请求
    srv.Delete(R"(/video/(\d+))",VideoDelete);//正则表达式，\d表示匹配一个数字字符
            //+表示匹配字符一次或多次 
            //R"(string)"去除括号中字符串中每个字符的特殊含义
    srv.Put(R"(/video/(\d+))",VideoUpdate);
    srv.Get(R"(/video)",VideoGetAll);
    srv.Get(R"(/video(\d+))",VideoGetOne);
    srv.Post(R"(/video)",VideoUpload);
    srv.Get(R"(/play/(\d+))",VideoPlay);
    srv.listen("0.0.0.0",9000);//监听

    return 0;
}

/*//插入测试
void test()
{
    vod_system::TableVod tb_vod;
    Json::Value val;
    val["name"] = "电锯惊魂2";
    val["vdesc"] = "这是个比较净损的电影";
    val["video_url"] = "/video/saw2.mp4";
    val["image_url"] = "/video/saw2.jpg";
    tb_vod.Insert(val);
    return;
}*/

/*
void test()//查询测试
{
    vod_system::TableVod tb_vod;
    Json::Value val;
    tb_vod.GetOne(3,&val);
    Json::StyledWriter writer;
    std::cout<<writer.write(val)<<std::endl;

    return;
    
}*/

/*
void test()//删除测试
{
    vod_system::TableVod tb_vod;
    Json::Value val;
    val["name"] = "电锯惊魂";
    val["vdesc"] = "这是个比较净损的电影";
    val["video_url"] = "/video/saw.mp4";
    val["image_url"] = "/video/saw.jpg";
    tb_vod.Delete(2);
    return;
}*/

/*
void test()//修改测试
{
    vod_system::TableVod tb_vod;
    Json::Value val;
    val["name"] = "电锯惊魂";
    val["vdesc"] = "这是个很血腥的电影";
    val["video_url"] = "/video/saw.mp4";
    val["image_url"] = "/video/saw.jpg";
    tb_vod.Update(2,val);
    return;
}*/



/*void test1()
{
    vod_system::TableVod tb_vod;
    Json::Value val;
    val["name"] = "速度与激情9";
    val["vdesc"] = "这是个非常刺激的电影";
    val["video_url"] = "/video/speed.mp4";
    val["image_url"] = "/video/speed.jpg";
    tb_vod.Insert(val);
    return;
}*/
/*
int main()
{
    //局部功能测试
    test();
    return 0;
}*/
