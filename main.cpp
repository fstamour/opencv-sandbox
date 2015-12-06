#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// Std
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

// Cv
using cv::Mat;
// using cv::UMat; OpenCV 3 only apparently
using cv::KeyPoint;
using cv::Scalar;
using cv::FileStorage;
using cv::FileNode;

// Check if an image is ok (check after loading for example).
// TODO Add a error description.
void check_img_ok(const cv::Mat& img) {
   if(img.empty()) {
        throw std::runtime_error("Image is empty");
   }
}

vector<cv::Point2i> readVectorOfPoint2i(const FileNode& fn) {
    vector<cv::Point2i> result;
    if (fn.type() != FileNode::SEQ) {
        cerr << "[readVectorOfPoint2i] FileNode is not a sequence! FAIL" << endl;
        exit(1);
    }
    for(FileNode coord : fn) {
        cv::Point2i p;
        p.x = (int)coord["x"];
        p.y = (int)coord["y"];
        result.push_back(p);
    }
    return result;
}

void print_usage() {
    cout << "Usage: ./main filename" << endl;
}

int main(int argc, char** argv)
{
	string filename;
    if(argc == 2) {
        filename = argv[1];
    } else {
        print_usage();
        exit(1);
    }

    // Read metadata
    FileStorage fs("data.yaml", FileStorage::READ);
    FileNode fn = fs[filename];
    vector<cv::Point2i> filler = readVectorOfPoint2i(fn["filler"]);
    int thresh = (int)fn["threshold"];
    FileNode fn_crop = fn["crop"];
    int crop_x = fn_crop["x"];
    int crop_y = fn_crop["y"];
    int crop_width = fn_crop["width"];
    int crop_height = fn_crop["height"];
    /*cout << "Thresh: " << thresh << "\n"
        << "crop: " << crop_x << " " << crop_y << " " << crop_width << " " << crop_height << endl;*/
    
    // Open the image
    filename = "data/" + filename + ".jpg";
	Mat img = cv::imread(filename, cv::IMREAD_GRAYSCALE);
    check_img_ok(img);

    // Crop the image 
    // The "img" variable is used as a temporary variable.
    Mat(img, cv::Rect(crop_x, crop_y, crop_width, crop_height)).copyTo(img);
    Mat orig = img.clone();
    
    // Threshold
    cv::threshold(img, img, thresh, 255, cv::THRESH_BINARY);

    ///// Find Contours
    vector<vector<cv::Point>> contours;
    vector<cv::Vec4i> hierarchy;
    Mat img_contours = img.clone();
    findContours(img_contours, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
    vector<vector<cv::Point>> hull(contours.size());

    /// Find the convex hull object for each contour
    /*for( int i = 0; i < contours.size(); i++ ) {
        cv::convexHull( Mat(contours[i]), hull[i], false );
    }*/

    /// Draw contours + hull results
    /*cv::RNG rng(4447);
    Mat drawing = Mat::zeros( img.size(), CV_8UC3 );
    for( int i = 0; i< contours.size(); i++ ) {
        Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
        cv::drawContours( drawing, contours, i, color, 1, 8, vector<cv::Vec4i>(), 0, cv::Point() );
        cv::drawContours( drawing, hull, i, color, 1, 8, vector<cv::Vec4i>(), 0, cv::Point() );
    }*/
    
    /// Find the largest contour
    size_t max = 0;
    size_t max_index = 0;
    size_t i = 1;
    for(; i < contours.size(); ++i) {
        double area = cv::contourArea(contours[i]);
        if(area > max) {
            max = area;
            max_index = i;
        }
    }

    // Flood fill the exterior
    Mat mask(img);
    for(cv::Point2i point : filler) {
        cv::floodFill(mask, point, Scalar(255));
    }
    //cv::bitwise_not(img_flooded, img_flooded);

    Scalar color = Scalar(0, 0, 0);
    cv::convexHull( Mat(contours[max_index]), hull[0], false );
    cv::drawContours(mask, hull, 0, color, -1, 8, vector<cv::Vec4i>(), 0, cv::Point());

    Mat result = orig | mask;
    //Mat result = img_flooded;

    /************ END OF "REGION OF INTEREST" EXTRACTION ************/
    
    // Blobs detection
    /*cv::SimpleBlobDetector detector;
    std::vector<KeyPoint> keypoints;
    detector.detect(result, keypoints);

    Mat img_with_keypoints;
    cv::drawKeypoints(result, keypoints, img_with_keypoints, Scalar(0,0,255), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);*/
  
    ////////////////////////////////////
    /*Mat background = result.clone();
    int kernel_size = 20;
    Mat kernel = Mat::ones( kernel_size, kernel_size, CV_32F ) / (float)(kernel_size*kernel_size);
    
    /// Apply filter
    cv::Point anchor(-1, -1);
    int delta = 0;
    filter2D(background, background, -1, kernel, anchor, delta, cv::BORDER_DEFAULT );

    Scalar mean = cv::mean(background, mask);
    cout << (255 - mean[0]) << endl;*/
    //result -= ;
    ////////////////////////////////////
   
    Mat background;
    int dilatation_size = 2;
    Mat element = cv::getStructuringElement( cv::MORPH_RECT, cv::Size(2*dilatation_size+1, 2*dilatation_size+1), cv::Point(dilatation_size, dilatation_size));
    cv::dilate(result, background, element);
    result = background - result;
    
    string windowName = "MyWin";
    cv::namedWindow(windowName, CV_WINDOW_AUTOSIZE);
    cv::imshow(windowName, result);
    cv::waitKey(0);
    cv::destroyWindow(windowName);
    return 0;
}
