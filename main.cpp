/*
    Title: Non-Intrusive Eye Direction Classification and Blink Detection using Video Analysis
    Descripion:
    The face of the subject is first detected using the YCbCr color space and connected-component analysis.
    The eye region will then be localized by following some rules in face geometry.
    Each iris and eye boundary is segmented from the region of interest by filtering, intensity thresholding and morphological operations.
    After detecting the eye, eye direction classification and blink detection is performed by obtaining the center of mass of the iris and feeding it to direction indicator.
    No device other than a camera and computer is needed using the above approach.
*/

#define IMAGE_RANGE_CHECK
#include "stdio.h"
#include "image.h"
#include "jpegio.h"
#include "binary.h"
#include "color.h"
#include "filter.h"
#define SIZE 450

/* Global Variables */
int centerAdjust=0;
int MARGIN=0;
float irisThreshold, eyeThreshold;
int flag=0, overAllBlack=0, firstEyeHeight[2], firstMaxPupilHeight[2];
int boxWidth=225, boxHeight=225;
int graphX=0;
int blink=0, upperLeft=0, up=0, upperRight=0, right=0, center=0, left=0, lowerLeft=0, low=0, lowerRight=0;
int leftFlag, rightFlag, centerFlag, blinkFlag; 
int centerx, centery;   // center of mass
int prevEyeStartX=0, prevEyeStartY=0, prevEyeWidth=0, prevEyeHeight=0;
int previousY=60;


/* Function for scaling images */
void scaleRGB( RGBImage & outputImage, const RGBImage & inputImage) {
	int xTarget,yTarget,xSource,ySource;
	int width = inputImage.width();
	int height = inputImage.height();
	int  targetWidth = width*3;
	int targetHeight = height*3;
  
	outputImage.resize(targetWidth,targetHeight);

	for (xTarget = 0; xTarget < targetWidth; xTarget++) {
		for (yTarget = 0; yTarget < targetHeight; yTarget++) {
			xSource = xTarget * width / targetWidth;
			ySource = yTarget * height / targetHeight;
			outputImage(xTarget,yTarget) = inputImage(xSource,ySource);
		}
	}
}

/* Function for coloring the boxes in eye direction */
void colorEyeDirection(RGBImage & eyeDirection, int startWidth, int endWidth, int startHeight, int endHeight){
	int x,y;
	for(x=startWidth; x<endWidth; x++){
		for(y=startHeight; y<endHeight; y++){
			eyeDirection(x,y) = COLOR_RGB(0,128,0);
		}
	}
}

int drawGraph(RGBImage & graph, int startY){
	int x,y;
	if(leftFlag){
		y=80;
	}
	else if(centerFlag){
		y=60;
	}
	else if(rightFlag){
		y=40;
	}
	else if(blinkFlag){
		y=20;
	}
	
	/* to change from one direction to another */
	if(y>startY){		
		for(int y1=startY+1; y1<=y; y1++)			
			graph(graphX,y1)=COLOR_RGB(255,128,0);
	}
	else{
		for(int y1=y+1; y1<=startY; y1++)			
			graph(graphX,y1)=COLOR_RGB(255,128,0);
	}
	if(blinkFlag){
		for(x=graphX; x<=graphX+1; x++)
			graph(x,y)=COLOR_RGB(0,255,0);
	}
	else{
		for(x=graphX; x<=graphX+1; x++)
			graph(x,y)=COLOR_RGB(255,128,0);
	}	
	return y;
}

void centerOfMass(RGBImage & eye, int width, int height){ 
	int x, y;
	int pix;
	
	int momentx=0, momenty=0, counterBlack=0;
	for(x=0; x<width; x++){
		for(y=0; y<height; y++){
			pix=eye(x,y);
			if(pix==COLOR_RGB(0,0,0)){
				momentx += x;
				momenty += y;
				counterBlack++;
			}
		}
	}
	
	centerx = (int)(momentx/counterBlack);
	centery = (int)(momenty/counterBlack);
	
	for(x=centerx-1; x<=centerx+1; x++){
		for(y=centery-1; y<=centery+1; y++){
			eye(x, y) = COLOR_RGB(0, 0, 255);
		}
	}
}

/* Function to determine the movement or blink of the eye */
void eyeMovement(RGBImage & eye, RGBImage & eyeDirection, int width, int height, int i, int eyeNum, int realEyeNum, int maxPupilHeight, int eyeHeight){
	int x, y;
	int box1, box2, box3, box4;
	box1=box2=box3=box4=0;
	leftFlag = centerFlag = rightFlag = blinkFlag = 0;
	eyeDirection.resize(boxWidth, boxHeight);
	eyeDirection.setAll(COLOR_RGB(255,255,255));
	
	int adjust=((width/3)/5);
	
	//draw the 9 boxes
	for (x = 0; x < boxWidth;  x++){
		for (y=0; y<3; y++){
			eyeDirection(x, y+boxHeight/3) = COLOR_RGB(255,0,0);
			eyeDirection(x, y+boxHeight-boxHeight/3) = COLOR_RGB(255,0,0);
		}
	}
	for(x=0;x<3;x++){
		for (y = 0; y < boxHeight; y++){
			eyeDirection(x+boxWidth/3, y) = COLOR_RGB(255,0,0);
			eyeDirection(x+boxWidth-boxWidth/3, y) = COLOR_RGB(255,0,0);
		}
	}
	// get the total number of black pixels in the current frame
	int eyeBlack=0;
	for(x=0; x<width; x++){
		for(y=0; y<height; y++){
			if(eye(x,y) == COLOR_RGB(0,0,0))
				eyeBlack++;
		}
	}
	// blink
	if(eyeBlack <= overAllBlack/6){
		blinkFlag=1;
		if(realEyeNum==2) blink++;
		for(x=(boxWidth/3)+3; x<2*(boxWidth/3); x++){
			for(y=(boxHeight/3)+3; y<2*(boxHeight/3); y++){
				eyeDirection(x,y) = COLOR_RGB(128,0,0);
			}
		}		
	} 
	else{ 
		centerOfMass(eye, width, height); 
			/* Upper Left */
			if(centerx<width/3 + adjust -centerAdjust && centery<(height/3)){	
				leftFlag=1;
				colorEyeDirection(eyeDirection,0,boxWidth/3,0,boxHeight/3);	
				if(realEyeNum==2) upperLeft++;
			}
			/* Left */
			else if(centerx<width/3 + adjust - centerAdjust && centery>=height/3&& centery<=2*(height/3)){
				leftFlag=1;
				colorEyeDirection(eyeDirection,0,boxWidth/3,3+boxHeight/3,2*(boxHeight/3));	
				if(realEyeNum==2) left++;
			}
			/* Lower Left */
			else if(centerx<width/3 + adjust - centerAdjust && centery>2*(height/3)){ 
				leftFlag=1;
				colorEyeDirection(eyeDirection,0,boxWidth/3,3+2*(boxHeight/3),boxHeight);
				if(realEyeNum==2) lowerLeft++; 
			}
			/* Top */
			else if(centerx>=width/3 + adjust - centerAdjust && centerx<2*(width/3) -adjust && centery<height/3){ 
				centerFlag=1;
				colorEyeDirection(eyeDirection,3+boxWidth/3,2*(boxWidth/3),0,boxHeight/3);
				if(realEyeNum==2) up++;
			}
			/* Center */
			else if((centerx>=width/3 + adjust - centerAdjust) && (centerx<2*(width/3) - adjust) && centery>=height/3 && centery<=2*(height/3)){ 
				centerFlag=1;
				colorEyeDirection(eyeDirection,3+boxWidth/3,2*(boxWidth/3),3+boxHeight/3,2*(boxHeight/3));
				if(realEyeNum==2) center++;
			}
			/* Bottom */
			else if(centerx>=width/3 + adjust - centerAdjust && centerx<2*(width/3) - adjust && centery>2*(height/3)){
				centerFlag=1;
				colorEyeDirection(eyeDirection,3+boxWidth/3,2*(boxWidth/3),3+2*(boxHeight/3),boxHeight);
				if(realEyeNum==2) low++;
			}
			/* Upper Right */
			else if(centerx>=2*(width/3) - adjust && centery<height/3){
				rightFlag=1;
				colorEyeDirection(eyeDirection,3+2*(boxWidth/3),boxWidth,0,boxHeight/3);
				if(realEyeNum==2) upperRight++;
			}
			/* Right */
			else if(centerx>=2*(width/3) - adjust && centery>=height/3 && centery<=2*(height/3)){ 
				rightFlag=1;
				colorEyeDirection(eyeDirection,3+2*(boxWidth/3),boxWidth,3+boxHeight/3,2*(boxHeight/3));
				if(realEyeNum==2) right++;
			}
			/* Lower Right */
			else if(centerx>=2*(width/3) - adjust && centery>2*(height/3)){ 
				rightFlag=1;
				colorEyeDirection(eyeDirection,3+2*(boxWidth/3),boxWidth,3+2*(boxHeight/3),boxHeight);
				if(realEyeNum==2) lowerRight++;
			}
			else{  // Center
				centerFlag=1;
				colorEyeDirection(eyeDirection,3+boxWidth/3,2*(boxWidth/3),3+boxHeight/3,2*(boxHeight/3));
				if(realEyeNum==2) center++;
			}
	}
	for (x = 0; x < width;  x++){
		eye(x, height/3) = COLOR_RGB(255,0,0);
		eye(x, height-height/3) = COLOR_RGB(255,0,0);
		
	}				
	for (y = 0; y < height; y++){
		eye(width/3 + adjust - centerAdjust, y) = COLOR_RGB(255,0,0);
		eye(width-width/3- adjust, y) = COLOR_RGB(255,0,0);
	}
}

/* Function to detect the eye */
void detectEye(RGBImage & inputImage, RGBImage & outputImage, RGBImage & eye, RGBImage & eyeResized, RGBImage & eyeDirection, RGBImage & graph, int startRow2X, int startRow2Y, int w, int h, int i){
	char filename[50];
	int area[2], eyeMove[2];
	int eyeNum=0, eyeRegionStart, eyeRegionEnd, eyeRegion;
	int x, y, startX, startY, c, gray, irisFlag=0;
	float Y, Cb, Cr;
	
	Image<unsigned char> binary, binary1, componentImage, grayImage, grayMedian, strucElem;
	RGBImage pupil, sample;
	ConnectedComponents cc;   // in binary.h, a class for connected component labelling
	int filterWidth = 9;   // the width of the filter
	//int strucWidth = 3; 			//for normal
	int strucWidth = 11;
	
	area[0] = area[1] = 0;
	
	// variables for getting the larger iris
	int minStartY=1000, minStartX=0, minWidthX=0, minHeightY=0, pupilStartX, pupilStartY, pupilWidth, pupilHeight;
	int eyeStartX=0, eyeStartY=0, eyeWidth=0, eyeHeight=0;
	int maxPupilWidth=0, maxPupilHeight=0, maxPupilX=0, maxPupilY=0;
	
	int red, green, blue;
	while(eyeNum<2){
		/* left or right eye? */
		if(eyeNum==0){		
			eyeRegion=0;
			eyeRegionStart = MARGIN;
			eyeRegionEnd = 0;
		}
		else{
			eyeRegion=w/2;
			eyeRegionStart =0;
			eyeRegionEnd = MARGIN;
		}
		
		// create a binary image using thresholding
		binary.resize( w/2, h+h/2 );
		binary.setAll( 0 );
		binary1.resize( w/2, h+h/2 );
		binary1.setAll( 0 );
		grayImage.resize( w/2,h+h/2 );  
		
		int notBlink=0;
		
		/* iris */
		for (x = eyeRegionStart; x < w/2-eyeRegionEnd;  x++) {
			for (y = 0; y < h+h/2; y++) {
				gray = (RED(inputImage(startRow2X+x+eyeRegion,startRow2Y+h+y)) + GREEN(inputImage(startRow2X+x+eyeRegion,startRow2Y+h+y)) + BLUE(inputImage(startRow2X+x+eyeRegion,startRow2Y+h+y))) / 3;
				grayImage(x,y) = gray;
			}
		}
	
		grayMedian = orderStatFilter( grayImage, filterWidth, 50 );
		sample.resize(w/2, h+h/2); 
		sample.setAll(COLOR_RGB(255,255,255));
		int pix;
		double r, g, b, hue, saturation, intensity;
		for (x = eyeRegionStart; x < w/2-eyeRegionEnd;  x++) {
			for (y = 0; y < h+h/2; y++) {
				sample.setPix(x,y,grayMedian(x,y),grayMedian(x,y),grayMedian(x,y));			
				pix = sample(x,y);
				r = RED(pix);
				g = GREEN(pix);
				b = BLUE(pix);
						
				RGBtoHSI(r, g, b, hue, saturation, intensity);
				if(intensity<irisThreshold){
					binary(x,y) = 1;
				}
			}
		}
		strucElem.resize(strucWidth,strucWidth);   // a square structuring element
		strucElem.setAll(1);
		
		binary = binaryDilation( binary, strucElem, strucWidth/2, strucWidth/2 );
		binary = binaryErosion( binary, strucElem, strucWidth/2, strucWidth/2 ); 
		
		for (x = eyeRegionStart; x < w/2-eyeRegionEnd;  x++) {
			for (y = 0; y < h+h/2; y++) {
				if(binary(x,y)){
					notBlink = 1;
					sample(x,y) = COLOR_RGB(0,0,0);
				}
				else
					sample(x,y) = COLOR_RGB(255,255,255);
			}
		}
		writeJpeg(sample, "images/SP/median/median.jpg", 100);
	
		int _maxPupilWidth=0, _maxPupilHeight=0, _maxPupilX=0, _maxPupilY=0;
		int _eyeStartX=0, _eyeStartY=0, _eyeWidth=0, _eyeHeight=0;
	
		if(notBlink==1){ 
			int ROI = (w/2)*(h+h/2);
			int minimumArea = ROI/180; // range of size of objects to be considered
			int maximumArea = ROI/40;  // 25
			
			cc.analyzeBinary( binary, EIGHT_CONNECTED );	
			
			for(c = 0; c < cc.getNumComponents(); c++){ 
				componentImage = cc.getComponentBinary( c );  
					 
				int ch = componentImage.height();  // height of bounding box
				int cw = componentImage.width();   // width of bounding box
				int m,n, numPix=0;

				for (m = 0; m < cw; m++)
					for (n = 0; n < ch; n++)
						numPix += componentImage(m,n);
						
						
				// if the size of the object is within the specified range
				if (minimumArea < numPix && numPix < maximumArea) {	
					cc.getBoundary(c,startX,startY,cw,ch);
					
					if((startX>MARGIN && startX+cw<w-MARGIN) && (startY-5>0 && startY+ch<h+h/2)){
						if(_maxPupilWidth*_maxPupilHeight<cw*ch){ 
							_maxPupilX = maxPupilX = startX;
							_maxPupilY = maxPupilY = startY;
							_maxPupilWidth = maxPupilWidth = cw;
							_maxPupilHeight = maxPupilHeight = ch;
						}
						irisFlag=1;
					}	
				}
				else{ 
					eye.resize(50,15);
					eye.setAll(COLOR_RGB(255,255,255));
					for(x=20; x<=MARGIN; x++){
						for(y=5; y<=10; y++){
							eye(x,y) = (COLOR_RGB(0,0,0));
						}
					}
				}
			}
			
			area[eyeNum] = _maxPupilWidth*_maxPupilHeight;
			// draw the top and bottom boundaries (square)
			for (x = _maxPupilX; x < _maxPupilX+_maxPupilWidth;  x++) {
				outputImage(startRow2X+x+eyeRegion,startRow2Y+_maxPupilY+h) = COLOR_RGB(0,255,0);                 //top
				outputImage(startRow2X+x+eyeRegion,startRow2Y+_maxPupilY+_maxPupilHeight+h) = COLOR_RGB(0,255,0); //bottom
			}
						
			// draw the left and right boundaries (square)
			for (y = _maxPupilY; y < _maxPupilY+_maxPupilHeight; y++) {
				outputImage(startRow2X+_maxPupilX+eyeRegion,startRow2Y+y+h) = COLOR_RGB(0,255,0);
				outputImage(startRow2X+_maxPupilX+_maxPupilWidth+eyeRegion,startRow2Y+y+h)=COLOR_RGB(0,255,0);
			} 
			
			if(flag==0)
				firstMaxPupilHeight[0] = _maxPupilHeight;
			else if(flag==1)
				firstMaxPupilHeight[1] = _maxPupilHeight;
			
			/* Eye boundary */
			int ch, cw;
			
			if(irisFlag){ // exclude blink
				binary1.setAll(0);
				
				if(maxPupilY-5>0){
					for (x = eyeRegionStart; x < w/2-eyeRegionEnd;  x++) { 
						for (y = maxPupilY-5; y < h+h/2 ; y++) {
							pix = inputImage(startRow2X+x+eyeRegion,startRow2Y+h+y);
							r = RED(pix);
							g = GREEN(pix);
							b = BLUE(pix);
											
							RGBtoHSI(r, g, b, hue, saturation, intensity);
							//printf("\n%f, %f, %f", hue, saturation, intensity);
							if(intensity<eyeThreshold){
								binary1(x,y) = 1;
							}
						}
					} 
					
					
				strucElem.resize(strucWidth,strucWidth);   // a square structuring element
				strucElem.setAll(1);
				binary1 = binaryDilation( binary1, strucElem, strucWidth/2, strucWidth/2 );
				//binary1 = binaryDilation( binary1, strucElem, strucWidth/2, strucWidth/2 );
				binary1 = binaryErosion( binary1, strucElem, strucWidth/2, strucWidth/2 );
					
				
					minimumArea = ROI/68;
					maximumArea = ROI/10; 
				
					sample.resize(w/2, h+h/2); 
					sample.setAll(COLOR_RGB(255,255,255));
					for (x = eyeRegionStart; x < w/2-eyeRegionEnd;  x++) {
						for (y = 0; y < h+h/2; y++) {
							if(binary1(x,y)){
								sample(x,y) = COLOR_RGB(0,0,0);
							}
							else
								sample(x,y) = COLOR_RGB(255,255,255);
						}
					}
					writeJpeg(sample, "images/SP/median/median.jpg", 100);
				
					cc.analyzeBinary( binary1, EIGHT_CONNECTED );
				
					int maxEyeWidth=0, maxEyeHeight=0;
					for(c = 0; c < cc.getNumComponents(); c++){
						componentImage = cc.getComponentBinary( c );  
							 
						ch = componentImage.height();  // height of bounding box
						cw = componentImage.width();   // width of bounding box
						int m,n, numPix=0;				
						
						for (m = 0; m < cw; m++)
							for (n = 0; n < ch; n++)
								numPix += componentImage(m,n);
						// if the size of the object is within the specified range
						if (minimumArea <= numPix && numPix <= maximumArea) { 
							cc.getBoundary(c,startX,startY,cw,ch);
							if(startY-5>0){
								if(_eyeWidth*_eyeHeight < cw*ch){
									_eyeStartX = eyeStartX=startX;
									_eyeStartY = eyeStartY=startY;
									_eyeWidth = eyeWidth = cw;
									_eyeHeight = eyeHeight = ch;
								}
							}
						}
					}
					for (x = _eyeStartX; x < _eyeStartX+_eyeWidth;  x++) {
						outputImage(startRow2X+x+eyeRegion,startRow2Y+_eyeStartY+h) = COLOR_RGB(0,0,255);             //top
						outputImage(startRow2X+x+eyeRegion,startRow2Y+_eyeStartY+_eyeHeight+h) = COLOR_RGB(0,0,255);  //bottom
					}

					// draw the left and right boundaries (square)
					for (y = _eyeStartY; y < _eyeStartY+_eyeHeight; y++) {
						outputImage(startRow2X+_eyeStartX+eyeRegion,startRow2Y+y+h) = COLOR_RGB(0,0,255);
						outputImage(startRow2X+_eyeStartX+_eyeWidth+eyeRegion,startRow2Y+y+h)=COLOR_RGB(0,0,255);
					}	
					
					if(_eyeWidth==0 || _eyeHeight==0){
						eye.resize(50,15);
						eye.setAll(COLOR_RGB(255,255,255));
						for(x=20; x<=MARGIN; x++){
							for(y=5; y<=10; y++){
								eye(x,y) = COLOR_RGB(0,0,0);
							}
						}
					}
					else{
						grayImage.resize(_eyeWidth, _eyeHeight);
						grayImage.setAll(COLOR_RGB(255,255,255));
						
						eye.resize(_eyeWidth,_eyeHeight);
						eye.setAll(COLOR_RGB(255,255,255));
						
						for (x = eyeRegionStart; x < w/2-eyeRegionEnd;  x++) {
							for (y = 0; y < h+h/2; y++) {
								if(binary1(x,y)){
									sample(x,y) = COLOR_RGB(0,0,0);		
								}
								else
									sample(x,y) = COLOR_RGB(255,255,255);
							}
						}
						writeJpeg(sample, "images/SP/median/median.jpg", 100);
							if(abs(_maxPupilX-_eyeStartX)<_eyeWidth && abs(maxPupilY-_eyeStartY)<_eyeHeight){
								for (x = abs(_maxPupilX-_eyeStartX); x < abs(_maxPupilX-_eyeStartX+_maxPupilWidth);  x++){
									for (y = abs(_maxPupilY-_eyeStartY); y < abs(_maxPupilY-_eyeStartY+_maxPupilHeight); y++){
										eye(x,y) = COLOR_RGB(0,0,0);  
										if(flag==0) 
											overAllBlack++;
										if(flag==1)
											overAllBlack++;
									}	
								}
							}
							if(flag==0){
								firstEyeHeight[0] = _eyeHeight;
							}
							if(flag==1){
								firstEyeHeight[1] = _eyeHeight;
							}
							flag++;
							if(flag==2){
								overAllBlack = overAllBlack/2;
							}
						writeJpeg(eye, "images/SP/median/eye.jpg", 100);
					}
				}
				else{
					eye.resize(50,15);
					eye.setAll(COLOR_RGB(255,255,255));
					for(x=20; x<=MARGIN; x++){
						for(y=5; y<=10; y++){
							eye(x,y) = (COLOR_RGB(0,0,0));
						}
					}
				}
			}
			else{	// blink
				eye.resize(50,15);
				eye.setAll(COLOR_RGB(255,255,255));
			}
			
			
		}
		else{	// blink
			eye.resize(50,15);
			eye.setAll(COLOR_RGB(255,255,255));
		} 
		
		// display the original eye
		sprintf(filename, "images/SP/eye/%d/%d.jpg", eyeNum, i);
	    writeJpeg( eye, filename, 100 );
		
		eyeMovement(eye, eyeDirection, eye.width(), eye.height(), i, eyeNum, eyeNum, _maxPupilHeight, _eyeHeight);
		
		sprintf(filename, "images/SP/eyeDirection/%d/%d.jpg", eyeNum, i);
		writeJpeg( eyeDirection, filename, 100 );
		
		scaleRGB(eyeResized, eye);
		int cw = eyeResized.width();
		int ch = eyeResized.height(); 
		// display the resized eye
		sprintf(filename, "images/SP/eyeResized/%d/%d.jpg", eyeNum, i);
	    writeJpeg( eyeResized, filename, 100 );

		eyeNum++;
	}
	printf("\n%d", eyeNum);
	if(area[0]>area[1]){
		sprintf(filename, "images/SP/eye/%d/%d.jpg", 0, i);
		readJpeg(eye, filename);
		eyeMovement(eye, eyeDirection, eye.width(), eye.height(), i, 0, eyeNum, maxPupilHeight, eyeHeight);
	}else{
		sprintf(filename, "images/SP/eye/%d/%d.jpg", 1, i);
		readJpeg(eye, filename);
		eyeMovement(eye, eyeDirection, eye.width(), eye.height(), i, 1, eyeNum, maxPupilHeight, eyeHeight);
	}	
	previousY = drawGraph(graph, previousY);
	sprintf(filename, "images/SP/graph/%d.jpg",  i);
	writeJpeg( graph, filename, 100 );
	
	graphX = graphX+2;
}

/* Function to detect the face */
void detectFace(RGBImage & inputImage, RGBImage & face, RGBImage & eye, RGBImage & eyeResized, RGBImage & eyeDirection, RGBImage & graph, RGBImage & outputImage, int width, int height, int i){
	int x, y, startX, startY, cw, ch, w, h;
	int pix;
	double r, g, b;
	double maxY=0.0, maxCr=0.0, maxCb=0.0;
	double hue, saturation, intensity;
	float Y, Cb, Cr;
	int strucWidth = 11;
	
	double saturationAdjust = 0.5;   // adjustment range is -1.0 to 1.0
    double intensityAdjust = -0.25;  // adjustment range is -1.0 to 1.0
	
	Image<unsigned char> binary, strucElem, componentImage;
	
	face.resize(width, height);
	face.setAll(0);
	binary.resize( width, height );
    binary.setAll(0);
	
	for(x=0; x<width; x++){
		for(y=0; y<height; y++){
			pix = inputImage(x,y);
			r = RED(pix)/255.0;
			g = GREEN(pix)/255.0;
			b = BLUE(pix)/255.0;
			
			Y = 0.299*r + 0.587*g + 0.114*b;
			Cr = 0.7132* fabs(r - Y);
			Cb = 0.5647*fabs(b -Y);
			
			if(maxY<Y)
				maxY = Y;
			if(maxCr<Cr)
				maxCr = Cr;
			if(maxCb<Cb){
				maxCb = Cb;
			}
		}
	}
	
	for (x = 0; x < width;  x++) {
		for (y = 0; y < height; y++) {
		    pix = inputImage(x,y);
		    // get the red, green, and blue components
		    r = RED(pix)/255.0;
		    g = GREEN(pix)/255.0;
		    b = BLUE(pix)/255.0;
		   
		    Y = (0.299*r + 0.587*g + 0.114*b);
			Cr = 0.7132* fabs((r - Y));
			Cb = 0.5647* fabs((b -Y));
			
			Y = Y/maxY * 255;
			Cr = Cr/maxCr * 255;
			Cb = Cb/maxCb * 255;
			if(Y>50 && Cb>=60 && Cb<=250 && Cr>=50 && Cr<=250){
				binary(x,y)=1;
			}
		}
   }
	strucElem.resize(strucWidth,strucWidth);   // A square structuring element
	strucElem.setAll(1);
	
	binary = binaryErosion( binary, strucElem, strucWidth/2, strucWidth/2 );
	binary = binaryDilation( binary, strucElem, strucWidth/2, strucWidth/2 );
	binary = binaryDilation( binary, strucElem, strucWidth/2, strucWidth/2 );
	
	
	for (x = 0; x < width;  x++) {
		for (y = 0; y < height; y++) {
			if (binary(x,y)) {
				face(x,y) = COLOR_RGB(255,255,255);
			}
		}
	}

	ConnectedComponents cc;
	cc.analyzeBinary( binary, EIGHT_CONNECTED );
	int area = 0;
	
	for(int c = 0; c < cc.getNumComponents(); c++){
		componentImage = cc.getComponentBinary( c );  
			 
		int ch = componentImage.height();  // height of bounding box
		int cw = componentImage.width();   // width of bounding box

		if(area < cw*ch){	//get the biggest component image
			area = cw*ch;
			cc.getBoundary(c,startX,startY,w,h);
		}
	}	
	/* Box the face */
	for (x = startX; x < startX+w;  x++) {
		outputImage(x,startY) = COLOR_RGB(255,0,0);     //top
		outputImage(x,startY+h-1) = COLOR_RGB(255,0,0); //bottom
	}
	for (y = startY; y < startY+h;  y++) {
		outputImage(startX,y) = COLOR_RGB(255,0,0);     //left
		outputImage(startX+w-1,y) = COLOR_RGB(255,0,0); //right
	}
	//bound the eye
	int eyeHeight = h/6;
	for (x = startX; x < startX+w;  x++) {
		outputImage(x,startY+eyeHeight) = COLOR_RGB(255,0,0);                //upper bound
		outputImage(x,startY+2*eyeHeight+eyeHeight/2) = COLOR_RGB(255,0,0);  //lower bound
	}
	for(y=startY+eyeHeight; y<startY+2*eyeHeight+eyeHeight/2; y++){
		outputImage(startX+w/2,y) = COLOR_RGB(255,0,0);  //middle line
	}
	detectEye(inputImage, outputImage, eye, eyeResized, eyeDirection, graph, startX, startY, w, eyeHeight, i); 
}

int main () {
	int i, x, y;
	int height, width;
	char filename[50];	
	char input[20];
	int lighting;
	RGBImage inputImage, outputImage, face, finalOutputImage, eyeResized;
	RGBImage  eye, eyeDirection;
	RGBImage label, title, leftTitle, rightTitle;
	RGBImage graph;
	
	title.resize(325,100);
	title.setAll(COLOR_RGB(0,0,0));

	printf("\n\n------------------------------");
	printf("\n Eye Gaze and Blink Detection");
	printf("\n------------------------------");
	printf("\n\nEnter Folder Name: ");
	scanf("%s", &input);
	printf("\n\n1. Bright\n2. Bright Near\n3. Normal\n4: Normal-Controlled\n5: Uneven");
	printf("\nSelect the corresponding lighting condition: ");
	scanf("%d", &lighting);
	
	graph.resize(SIZE*2, 100);
	graph.setAll(COLOR_RGB(0,0,0));
	
	switch(lighting){
		case 1: irisThreshold = 0.15;
				eyeThreshold = 0.35;
				centerAdjust = 2;
				MARGIN=20;
				break;
		case 2: irisThreshold = 0.16;
				eyeThreshold = 0.35;
				centerAdjust = 0;
				MARGIN=25;
				break;
		case 3: irisThreshold = 0.12;
				eyeThreshold = 0.25;
				centerAdjust = 1;
				MARGIN=30;
				break;
		case 4: irisThreshold = 0.06;
				eyeThreshold = 0.20;
				centerAdjust = 1;
				MARGIN=15;
				break;
		case 5: irisThreshold = 0.05;
				eyeThreshold = 0.17;
				centerAdjust = 3;
				MARGIN=30;
				break;
		default: printf("Please select from one of the following options."); 
				break;
	}
	
	readJpeg( label, "images/SP/label.jpg");
	readJpeg( title, "images/SP/title.jpg");
	readJpeg( leftTitle, "images/SP/left.jpg");
	readJpeg( rightTitle, "images/SP/right.jpg");
	
    for(i=0; i<SIZE; i++){
		leftFlag=rightFlag=centerFlag=blinkFlag=0;
		
		sprintf(filename, "images/SP/input/%s/%d.jpg", input, i);
	    readJpeg( inputImage, filename);
	    height = inputImage.height();
	    width  = inputImage.width();
	   
	    outputImage.resize(width, height);
		outputImage = inputImage;
	    
		detectFace(inputImage, face, eye, eyeResized, eyeDirection, graph, outputImage, width, height, i); 
		sprintf(filename, "images/SP/face/%d.jpg", i);
	    writeJpeg( face, filename, 100 ); 
		
		sprintf(filename, "images/SP/output/%d.jpg", i);
		writeJpeg( outputImage, filename, 100 );
		
		finalOutputImage.resize(width+2*boxWidth+100, height+200);
		finalOutputImage.setAll(0);
		
		for(int x=0; x<690; x++){            // draw title
			for(int y=0; y<100; y++){
				finalOutputImage(x+250,y) = title(x,y);
			}
		}
		
		for(int x=0; x<275; x++){			// draw title
			for(int y=0; y<100; y++){
				finalOutputImage(x,y+100) = leftTitle(x,y);
			}
		}
		
		for(int x=0; x<275; x++){			// draw title
			for(int y=0; y<100; y++){
				finalOutputImage(boxWidth+50+width+x,y+100) = rightTitle(x,y);
			}
		}
		
		for(int x=0; x<width; x++){			// draw output image
			for (int y = 0; y < height;  y++){
				finalOutputImage(x+boxWidth+50,y+100) = outputImage(x,y);
			}
		}
		
		sprintf(filename, "images/SP/eyeDirection/0/%d.jpg", i);
		readJpeg( eyeDirection, filename);
		for(int x=0; x<boxWidth; x++){      // draw eye direction
			for (int y = 0; y < boxHeight;  y++){
				finalOutputImage(x+25,y+200) = eyeDirection(x,y);
			}
		}
		
		sprintf(filename, "images/SP/eyeResized/0/%d.jpg", i);
		readJpeg( eyeResized, filename);
		int eyeWidth = eyeResized.width();
		int eyeHeight = eyeResized.height();
		for(int x=0; x<eyeWidth; x++){       // draw eye
			for (int y = 0; y < eyeHeight;  y++){
				finalOutputImage(x+65,boxHeight+y+230) = eyeResized(x,y);
			}
		}
		
		sprintf(filename, "images/SP/eyeDirection/1/%d.jpg", i);
		readJpeg( eyeDirection, filename);
		for(int x=0; x<boxWidth; x++){      // draw eye direction
			for (int y = 0; y < boxHeight;  y++){
				finalOutputImage(boxWidth+width+x+75,y+200) = eyeDirection(x,y);
			}
		}
		
		sprintf(filename, "images/SP/eyeResized/1/%d.jpg", i);
		readJpeg( eyeResized, filename);
		eyeWidth = eyeResized.width();
		eyeHeight = eyeResized.height();
		for(int x=0; x<eyeWidth; x++){      // draw eye
			for (int y = 0; y < eyeHeight;  y++){
				finalOutputImage(boxWidth+width+x+115,boxHeight+y+230) = eyeResized(x,y);
			}
		}
		
		for(int x=0; x<SIZE*2; x++){        // draw graph
			for (int y = 0; y < 100;  y++){
				finalOutputImage(x+65,height+y+100) = graph(x,y);
			}
		}
		
		for(int x=0; x<65; x++){			
			for (int y = 0; y < 100;  y++){
				finalOutputImage(x,height+y+100) = label(x,y);
			}
		}
		
	   // write the output to a JPEG file
	   sprintf(filename, "images/SP/finalOutput/%d.jpg", i);
	   writeJpeg( finalOutputImage, filename, 100 ); 
	   
		
	}
	/* Display result */
	printf("\n\n -------------------");
	printf("\n Summary of Results:");
	printf("\n -------------------");
		
	printf("\n\n Eye Direction:\n");
	printf("\n		   Left");
	printf("\n   * Upper Left  |  %d  ", upperLeft);
	printf("\n   * Left        |  %d  ", left);
	printf("\n   * Lower Left  |  %d  ", lowerLeft);
	printf("\n   * Upper       |  %d  ", up);
	printf("\n   * Center      |  %d  ", center);
	printf("\n   * Lower       |  %d  ", low);
	printf("\n   * Upper Right |  %d  ", upperRight);
	printf("\n   * Right       |  %d  ", right);
	printf("\n   * LowerRight  |  %d  \n\n", lowerRight);
	
	printf("\n   * Blink       |  %d  |\n", blink);
}
