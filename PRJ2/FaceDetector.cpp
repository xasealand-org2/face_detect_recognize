#include "FaceDetector.h"
#include <cmath>
#include <algorithm>
#define PI 3.1415926
FaceDetector::FaceDetector() {

}
FaceDetector::~FaceDetector() {

}
bool FaceDetector::load(string facePath, string eyePath) {
	if (face.load(facePath) && eye.load(eyePath)) {
		return true;
	}
	cerr << "无法加载检测模型!" << endl;
	return false;
}
void FaceDetector::detect(CascadeClassifier &model, Mat &grayImg, vector<Rect> &result,
	double scale, int minNeibor, Size minSize,
	Size maxSize) {
	model.detectMultiScale(grayImg, result, scale, minNeibor, CV_HAAR_SCALE_IMAGE, minSize, maxSize);
}
bool FaceDetector::getFaceRect(Mat &img, vector<Rect> &result, 
	vector<Mat> &aligned, size_t len) {
	Mat gray;
	cvtColor(img, gray, COLOR_BGR2GRAY);
	equalizeHist(gray, gray);//增加对比度
	detect(face, gray, result, 1.5, 3, Size(gray.cols * 0.1, gray.rows * 0.1));
	if(result.size() > len)
		result.erase(result.begin() + len);//最多只检测len个对象
	vector<Rect> eyePos;
	for (int i = 0; i < result.size(); i++) {
		eyePos.clear();

		//在人脸区域中检测人眼
		gray = gray(result[i]);
		detect(eye, gray, eyePos, 1.1, 3,
			Size(), Size(result[i].width * 0.4, result[i].height * 0.4));
		if (eyePos.size() < 2) {
			clog << "警告: 未检测到人眼!" << endl;
			return false;
		}
		Point2f pos0(eyePos[0].x + eyePos[0].width / 2.0,
			eyePos[0].y + eyePos[0].height / 2.0);
		Point2f pos1(eyePos[1].x + eyePos[1].width / 2.0,
			eyePos[1].y + eyePos[1].height / 2.0);

		//框出双眼
		rectangle(img, eyePos[0] + Point(result[i].x, result[i].y), Scalar(0, 0, 255));
		rectangle(img, eyePos[1] + Point(result[i].x, result[i].y), Scalar(0, 0, 255));

		Point2f cen = (pos0 + pos1) / 2;
		Point2f dir = pos0 - cen; 
		double r = sqrt((cen.x - pos0.x)*(cen.x - pos0.x)
			+ (cen.y - pos0.y)*(cen.y - pos0.y));//双眼间距的一半

		//计算双眼所在直线与水平线的夹角,角度制
		double cosv = dir.ddot(Point2f(1, 0)) / 
			sqrt(dir.x * dir.x + dir.y * dir.y);
		double theta = acos(cosv) * 180 / PI;
		/*旋转不超过90度，图像的左边高，逆时针旋转；
		右边高，顺时针旋转*/
		if (theta > 90)
			theta = 180 - theta;
		bool b0 = cosv > 0, b1 = dir.y > 0;
		if (b0 && !b1 || !b0 && b1)
			theta = -theta;//保证旋转方向正确
		/*将人脸绕双眼中点旋转至水平；theta为正，
		逆时针旋转，为负，顺时针旋转*/
		Mat m = getRotationMatrix2D(cen, theta, 1);
		Mat rotated;
		warpAffine(gray, rotated, m, Size(gray.cols, gray.rows));
		//裁剪人脸
		Rect rect = cropFace(rotated, cen, r);
		aligned.push_back(rotated);
		//画出裁剪区域的位置
		drawCrop(img, RotatedRect(Point(result[i].x + rect.x + rect.width / 2,
			result[i].y + rect.y + rect.height / 2),
			rect.size(), theta));
	}
	return result.size() > 0;
}
Rect FaceDetector::cropFace(Mat &img, Point center, double r) {
	int x = max(0, (int)round(center.x - r*scaleHoriz));
	int y = max(0, (int)round(center.y - r*scaleUp));
	int width = min(img.cols - x, (int)round(2 * r*scaleHoriz));
	int height = min(img.rows - y, (int)round(r*(scaleUp + scaleDown)));
	img = img(Rect(x, y, width, height));
	return Rect(x, y, width, height);
}
void FaceDetector::drawCrop(Mat &img, RotatedRect &rot, Scalar color) {
	Point2f vertices[4];
	rot.points(vertices);
	for (int i = 0; i < 4; i++)
		line(img, vertices[i], vertices[(i + 1) % 4], color);
}