#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <opencv2/core/ocl.hpp>
#include <opencv2/core/utility.hpp>
#include "opencv2/imgcodecs.hpp"
#include <opencv2/video.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;
using namespace cv;

char *ex;

class App
{
public:
    App(CommandLineParser& cmd);
    void run();
    void handleKey(char key);
    void hogWorkBegin();
    void hogWorkEnd();
    string hogWorkFps() const;
    void workBegin();
    void workEnd();
    string workFps() const;
	
private:
    App operator=(App&);
    //Args args;
    bool running;
    bool make_gray;
    double scale;
    double resize_scale;
    int win_width;
    int win_stride_width, win_stride_height;
    int gr_threshold;
    int nlevels;
    double hit_threshold;
    bool gamma_corr;
    int64 hog_work_begin;
    double hog_work_fps;
    int64 work_begin;
    double work_fps;
    string img_source;
    string vdo_source;
    string output;
    int camera_id;
    bool write_once;
	
	bool save_frame;
	int frameIndex; 
	bool no_detect;
};


int main(int argc, char** argv)
{
    const char* keys =
        "{ h help      |                | print help message }"
        "{ i input     |                | specify input image}"
        "{ c camera    | -1             | enable camera capturing }"
        "{ v video     |	            | use video as input }"
        "{ g gray      |                | convert image to gray one or not}"
        "{ s scale     | 1.0            | resize the image before detect}"
        "{ o output    |   output.avi   | specify output path when input is images}"
		"{ n nodetect  |                | disable object detection (used for extracting images only}";		
			//Added -n option to avoid detection and just play so we can save images out of the video for training
		
    CommandLineParser cmd(argc, argv, keys);
    if (cmd.has("help")){
        cmd.printMessage();
        return EXIT_SUCCESS;
    }
	
	ex = argv[0];
	
    App app(cmd);
    try{
        app.run();
    }
    catch (const Exception& e){
        return cout << "error: "  << e.what() << endl, 1;
    }
    catch (const exception& e){
        return cout << "error: "  << e.what() << endl, 1;
    }
    catch(...){
        return cout << "unknown exception" << endl, 1;
    }
    return EXIT_SUCCESS;
}

App::App(CommandLineParser& cmd){
    cout << "\nControls:\n"
         << "\tESC - exit\n"
         << "\tm - change mode GPU <-> CPU\n"
         << "\tg - convert image to gray or not\n"
         << "\to - save output image once, or switch on/off video save\n"
		 << "\ts - save a single frame with no overlay for later object annotation\n"
         << "\t1/q - increase/decrease HOG scale\n"
         << "\t2/w - increase/decrease levels count\n"
         << "\t3/e - increase/decrease HOG group threshold\n"
         << "\t4/r - increase/decrease hit threshold\n"
         << endl;
    make_gray = cmd.has("gray");
    resize_scale = cmd.get<double>("s");
    vdo_source = samples::findFileOrKeep(cmd.get<string>("v"));
    img_source = cmd.get<string>("i");
    output = cmd.get<string>("o");
    camera_id = cmd.get<int>("c");
    win_width = 48;
    win_stride_width = 8;
    win_stride_height = 8;
    gr_threshold = 8;
    nlevels = 13;
    hit_threshold = 1.4;
    scale = 1.05;
    gamma_corr = true;
    write_once = false;
	
	save_frame = false;
	frameIndex = 1;
	no_detect = cmd.has("n");
	
    cout << "Group threshold: " << gr_threshold << endl;
    cout << "Levels number: " << nlevels << endl;
    cout << "Win width: " << win_width << endl;
    cout << "Win stride: (" << win_stride_width << ", " << win_stride_height << ")\n";
    cout << "Hit threshold: " << hit_threshold << endl;
    cout << "Gamma correction: " << gamma_corr << endl;
    cout << endl;
}
void App::run(){
    running = true;
    VideoWriter video_writer;
    Size win_size(win_width, win_width * 2);
    Size win_stride(win_stride_width, win_stride_height);
	
    // Loat HOG descriptor and detector here
	HOGDescriptor hog;
	
	
	//Get the executable path 
	string exPath = ".";
	size_t p = string(ex).rfind("/");
	if(p != string::npos)
		exPath = string(ex).substr(0, p);
	
	char rel[128];
	snprintf(rel, sizeof(rel), "%s/../train/ped64x128.xml", exPath.c_str());
	hog.load(rel);
	
	
	
    while (running){
        VideoCapture vc;
        UMat frame;
        if (vdo_source!=""){
            vc.open(vdo_source.c_str());
            if (!vc.isOpened())
                throw runtime_error(string("can't open video file: " + vdo_source));
            vc >> frame;
        }
        else if (camera_id != -1){
            vc.open(camera_id);
            if (!vc.isOpened())
            {
				
                stringstream msg;
                msg << "can't open camera: " << camera_id;
                throw runtime_error(msg.str());
            }
            vc >> frame;
        }
        else{
            imread(img_source).copyTo(frame);
            if (frame.empty())
                throw runtime_error(string("can't open image file: " + img_source));
        }
        UMat img_aux, img, img_to_show;
        // Iterate over all frames
        while (running && !frame.empty()){
			//resize the image frame so we don't have quite as much processing to do.
			Mat resized;
			resize(frame, resized, Size(960,540));
			
			if(!no_detect){
			
				workBegin();
				// Change format of the image
				if (make_gray) cvtColor(resized, img_aux, COLOR_BGR2GRAY );
				else resized.copyTo(img_aux);
				// Resize image
				if (abs(scale-1.0)>0.001)
				{
					Size sz((int)((double)img_aux.cols/resize_scale), (int)((double)img_aux.rows/resize_scale));
					resize(img_aux, img, sz, 0, 0, INTER_LINEAR_EXACT);
				}
				else img = img_aux;
				img.copyTo(img_to_show);
				hog.nlevels = nlevels;
				vector<Rect> found;
				vector<double> quality;
				// Perform HOG classification
				hogWorkBegin();
				//hog.detectMultiScale(img, found, hit_threshold, win_stride, Size(0, 0), scale, gr_threshold);
				hog.detectMultiScale(img, found, quality); 
				hogWorkEnd();
				// Draw positive classified windows
				for (size_t i = 0; i < found.size(); i++)
				{
					rectangle(img_to_show, found[i], Scalar(0, quality[i] * 255, 0), 3);
				}
				putText(img_to_show, ocl::useOpenCL() ? "Mode: OpenCL"  : "Mode: CPU", Point(5, 25), FONT_HERSHEY_SIMPLEX, 1., Scalar(255, 100, 0), 2);
				putText(img_to_show, "FPS (HOG only): " + hogWorkFps(), Point(5, 65), FONT_HERSHEY_SIMPLEX, 1., Scalar(255, 100, 0), 2);
				putText(img_to_show, "FPS (total): " + workFps(), Point(5, 105), FONT_HERSHEY_SIMPLEX, 1., Scalar(255, 100, 0), 2);
				imshow("opencv_hog", img_to_show);
				if (vdo_source!="" || camera_id!=-1) vc >> frame;
				workEnd();
				if (output!="" && write_once){
					if (img_source!="") {
						write_once = false;
						imwrite(output, img_to_show);
					}
					else {
						if (!video_writer.isOpened())
						{
							video_writer.open(output, VideoWriter::fourcc('x','v','i','d'), 24,
											  img_to_show.size(), true);
							if (!video_writer.isOpened())
								throw std::runtime_error("can't create video writer");
						}
						if (make_gray) cvtColor(img_to_show, img, COLOR_GRAY2BGR);
						else cvtColor(img_to_show, img, COLOR_BGRA2BGR);
						video_writer << img;
					}
				}
				
			}
			else{
				imshow("opencv_hog", resized);
				if (vdo_source!="" || camera_id!=-1) vc >> frame;
			}
				
			if(save_frame){
				save_frame = false;
				char name[64];
				snprintf(name, sizeof(name),"%s/frames/save_%04i.png", exPath.c_str(), frameIndex++);
				imwrite(name, resized);
				cout << "Saved image to " << name << endl;
			}
			
            handleKey((char)waitKey(no_detect ? 30 : 3));
		}
        
    }
}


void App::handleKey(char key)
{
    switch (key){
    case 27:
        running = false;
        break;
    case 'm':
    case 'M':
        ocl::setUseOpenCL(!cv::ocl::useOpenCL());
        cout << "Switched to " << (ocl::useOpenCL() ? "OpenCL enabled" : "CPU") << " mode\n";
        break;
    case 'g':
    case 'G':
        make_gray = !make_gray;
        cout << "Convert image to gray: " << (make_gray ? "YES" : "NO") << endl;
        break;
    case '1':
        scale *= 1.05;
        cout << "Scale: " << scale << endl;
        break;
    case 'q':
    case 'Q':
        scale /= 1.05;
        cout << "Scale: " << scale << endl;
        break;
    case '2':
        nlevels++;
        cout << "Levels number: " << nlevels << endl;
        break;
    case 'w':
    case 'W':
        nlevels = max(nlevels - 1, 1);
        cout << "Levels number: " << nlevels << endl;
        break;
    case '3':
        gr_threshold++;
        cout << "Group threshold: " << gr_threshold << endl;
        break;
    case 'e':
    case 'E':
        gr_threshold = max(0, gr_threshold - 1);
        cout << "Group threshold: " << gr_threshold << endl;
        break;
    case '4':
        hit_threshold+=0.25;
        cout << "Hit threshold: " << hit_threshold << endl;
        break;
    case 'r':
    case 'R':
        hit_threshold = max(0.0, hit_threshold - 0.25);
        cout << "Hit threshold: " << hit_threshold << endl;
        break;
    case 'c':
    case 'C':
        gamma_corr = !gamma_corr;
        cout << "Gamma correction: " << gamma_corr << endl;
        break;
    case 'o':
    case 'O':
        write_once = !write_once;
        break;
		
	case 's':
		save_frame = true;
		break;
    }
}
inline void App::hogWorkBegin(){
    hog_work_begin = getTickCount();
}
inline void App::hogWorkEnd(){
    int64 delta = getTickCount() - hog_work_begin;
    double freq = getTickFrequency();
    hog_work_fps = freq / delta;
}
inline string App::hogWorkFps() const{
    stringstream ss;
    ss << hog_work_fps;
    return ss.str();
}
inline void App::workBegin(){
    work_begin = getTickCount();
}
inline void App::workEnd(){
    int64 delta = getTickCount() - work_begin;
    double freq = getTickFrequency();
    work_fps = freq / delta;
}
inline string App::workFps() const{
    stringstream ss;
    ss << work_fps;
    return ss.str();
}