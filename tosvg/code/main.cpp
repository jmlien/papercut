#include <iostream>
#include <list>
#include <string>
#include <cstdio>
#include "opencvstd.h"
#include "simple_svg_1.0.0.hpp"
#include <sys/stat.h>
#include "CurveCSS.h"
using namespace std;


inline void contour2ply(vector<cv::Point2d>& contour, svg::Polygon& poly)
{
	int psize = contour.size();
	for (int j = 0; j < psize; j++) poly << svg::Point(contour[j].x, contour[j].y);
	poly.endBoundary();
}


inline string getImageName(const string & img_name)
{
	//
	string output;
	int dot_pos = img_name.rfind(".");
	int slash_pos = img_name.rfind("/");
	if (slash_pos == string::npos)
		output = img_name.substr(0, dot_pos);
	else
		output = img_name.substr(slash_pos + 1, dot_pos - slash_pos - 1);
	//

	return output;
}

//distance between p and line containing s and e
inline float dist(const cv::Point2d& s, const cv::Point2d& e, const cv::Point2d& p)
{
	float v_x=e.x-s.x;
	float v_y=e.y-s.y;
	float norm=sqrt(v_x*v_x+v_y*v_y);
	float n_x=v_y/norm;
	float n_y=-v_x/norm;

	float u_x=p.x-s.x;
	float u_y=p.y-s.y;

	return fabs(n_x*u_x+n_y*u_y);
}

void DouglasPeucker(const vector<cv::Point2d>& pts, int start, int end,
	                  vector<cv::Point2d>& simplified, float dp)
{
	const int ptsize=pts.size();
	if(start>end) end+=ptsize;
	if(end-start<2){
		simplified.push_back(pts[start]);
		return;
	}

	//find extreme pts
	const cv::Point2d & s=pts[start];
	const cv::Point2d & e=pts[end%ptsize];
	int new_e=start;
	float max_d=0;

	for(int i=start+1;i<end;i++)
	{
		const cv::Point2d & p = pts[i%ptsize];
		float d=dist(s,e,p);
		if(d>max_d)
		{
			max_d=d;
			new_e=i;
		}
	}

	//
	if(max_d>dp)
	{
		new_e=new_e%ptsize;
		end=end%ptsize;
		vector<cv::Point2d> A, B;
		DouglasPeucker(pts, start, new_e, A, dp);
		DouglasPeucker(pts, new_e, end, B, dp);
		A.insert(A.end(),B.begin(),B.end());
		simplified.swap(A);
	}
	else //the furthest point is too close
	{
		simplified.push_back(pts[start]);
	}
}

void DouglasPeucker(vector<cv::Point2d>& pts, float dp, bool external)
{
	//cout<<"- DouglasPeucker: original size="<<pts.size()<<endl;
	//find 4 extreme points
	int e=0,w=0,n=0,s=0;
	for(int i=1;i<pts.size();i++)
	{
		if(pts[i].x>pts[e].x){ e=i; }
		if(pts[i].x<pts[e].x){ w=i; }
		if(pts[i].y>pts[e].y){ n=i; }
		if(pts[i].y<pts[e].y){ s=i; }
	}

	//cout<<"n="<<n<<" e="<<e<<" s="<<s<<" w="<<w<<endl;

	//break pts into 4 parts
	//recursively identify the extreme points
	vector<cv::Point2d> ns, sn;

	if(s!=n)
	{
		DouglasPeucker(pts,n,s,ns,dp);
		DouglasPeucker(pts,s,n,sn,dp);
	}
	else //s==n
	{
		DouglasPeucker(pts,e,w,ns,dp);
		DouglasPeucker(pts,w,e,sn,dp);
	}

	//merge
	pts.swap(ns);
	pts.insert(pts.end(),sn.begin(),sn.end());

	//done
	//cout<<"- DouglasPeucker: simplified size="<<pts.size()<<endl;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		cout << " Usage: "<<argv[0]<<" -ct color_intensity_threshold image -st contour_size_threshold -smooth amout -invert" << endl;
		return -1;
	}

	unsigned int size_threshold  = 5;  //smallest contour size
	unsigned int color_threshold = 50; //smallest intensity
	float smooth = 1;                  //amount of smoothing
	float dp = 0;                    //Douglas Peucker parameter
	bool invert = false;               //invert black/white
	bool debug = false;                //output debugging information

	string img_filename;
	for (int i = 1; i < argc; i++)
	{
		if (string(argv[i]) == "-ct" && i + 1<argc)  color_threshold = atoi(argv[++i]);
		else if (string(argv[i]) == "-st" && i + 1 < argc)  size_threshold = atoi(argv[++i]);
		else if (string(argv[i]) == "-smooth" && i + 1<argc)  smooth = atof(argv[++i]);
		else if (string(argv[i]) == "-invert")  invert = true;
		else if (string(argv[i]) == "-simplify" && i + 1<argc)  dp = atof(argv[++i]);
		else if (string(argv[i]) == "-debug") debug = true;
		else img_filename = argv[i];
	}

	//cout << "loading " << argv[1] << endl;
	cv::Mat image = cv::imread(img_filename.c_str(), CV_LOAD_IMAGE_GRAYSCALE);   // Read the file

	if (!image.data)                              // Check for invalid input
	{
		cout << "! Error: Could not open or find the image" << std::endl;
		return -1;
	}

	if (debug)
	{
		cv::namedWindow("Input image", cv::WINDOW_AUTOSIZE);// Create a window for display.
		imshow("Input image", image);                       // Show our image inside it.
		cv::waitKey(0);                                     // Wait for a keystroke in the window
	}

	string img_name = getImageName(img_filename);

	//convert image to black/white image
	cv::Mat threshold_output(image.size(), CV_8UC1, cv::Scalar(0));
	{
		//Note: this function extracts all contours except those with size smaller than threshold

		cv::Mat src_gray;

		//negate the color
		if (invert) bitwise_not(image, src_gray);
		else src_gray = image;

		/// Detect edges using Threshold
		cv::threshold(src_gray, threshold_output, color_threshold, 255, cv::THRESH_BINARY);
	}

	if (debug)
	{
		cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);  // Create a window for display.
		imshow("Display window", threshold_output);              // Show our image inside it.
		cv::waitKey(0);                                          // Wait for a keystroke in the window
	}

	//extract contours
	{
		/// Find contours
		vector<vector<cv::Point> > contours;
		vector<vector<cv::Point2d> > smoothed_contours;
		vector<cv::Vec4i> hierarchy;

		cv::findContours(threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

		int csize = contours.size();

		int total_size=0;
		for (int i = 0; i < csize; i++)
		{
			vector<cv::Point2d> smoothed;
			//ResampleCurve(contours[i], contours[i], contours[i].size(), false);
			SimpleSmoothCurve(contours[i], smoothed, smooth, false);
			if(dp>0) DouglasPeucker(smoothed, dp, hierarchy[i][3] == -1);
			total_size+=smoothed.size();
			smoothed_contours.push_back(smoothed);
		}
		cout<<"- total vertex size="<<total_size<<endl;

		//cout << "csize=" << csize << endl;
		//For each i - th contour contours[i], the elements hierarchy[i][0], hiearchy[i][1], hiearchy[i][2], and hiearchy[i][3] are set to 0 - based indices in
		//contours of the next and previous contours at the same hierarchical level, the first child contour and the parent contour, respectively.If for the
		//contour i there are no next, previous, parent, or nested contours, the corresponding elements of hierarchy[i] will be negative.

		//create a svg file per polygon
		char svg_filename[256];
		sprintf(svg_filename, "%s.svg", img_name.c_str());
		svg::Dimensions dimensions(image.size().width, image.size().height);
		svg::Document doc(svg_filename, svg::Layout(dimensions, svg::Layout::TopLeft));

		for (int i = 0; i < csize; i++)
		{
			//cout << "contour[" << i << "] size = " << contours[i].size() << " has hie[0]=" << hierarchy[i][0] << " hie[1]=" << hierarchy[i][1] << " hie[2]=" << hierarchy[i][2] << " hie[3]=" << hierarchy[i][3] << endl;

			if (hierarchy[i][3] == -1) //external boundary
			{
				unsigned int csize = smoothed_contours[i].size();
				if (csize < size_threshold) continue; //this external boundary is too small... ignore

				//------------------------------------------------------------------
				//draw the external boundary
				svg::Polygon poly_bd(svg::Fill(svg::Color::Yellow), svg::Stroke(0.5, svg::Color::Black));
				contour2ply(smoothed_contours[i], poly_bd);


				//draw the hole boundaries of the external boundary
				int childid = hierarchy[i][2];

				while (childid != -1)
				{
					//svg::Polygon hole_bd(svg::Stroke(0.25, svg::Color::Green));
					if (contours[childid].size() >= size_threshold) //make sure that the hole boundary is big enough6
					{
						contour2ply(smoothed_contours[childid], poly_bd);
						//doc << hole_bd;
					}
					childid = hierarchy[childid][0]; //move to the previous child
				}
				//------------------------------------------------------------------
				doc << poly_bd;
			}
		}//end for i

		doc.save();
		cout << "- Saved " << svg_filename << endl;
	}

	return 0;
}
