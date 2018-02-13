// 
// impressionistDoc.cpp
//
// It basically maintain the bitmap for answering the color query from the brush.
// It also acts as the bridge between brushes and UI (including views)
//

#include <FL/fl_ask.H>

#include "impressionistDoc.h"
#include "impressionistUI.h"

#include "ImpBrush.h"

// Include individual brush headers here.
#include "PointBrush.h"
#include "LineBrush.h"
#include "CircleBrush.h"
#include "ScatteredPointsBrush.h"
#include "ScatterLinesBrush.h"
#include "ScatteredCirclesBrush.h"
#include "Highlighter.h"
#include <math.h>


#define DESTROY(p)	{  if ((p)!=NULL) {delete [] p; p=NULL; } }

ImpressionistDoc::ImpressionistDoc() 
{
	// Set NULL image name as init. 
	m_imageName[0]	='\0';	

	m_nWidth		= -1;
	m_ucBitmap		= NULL;
	m_ucPainting	= NULL;


	// create one instance of each brush
	ImpBrush::c_nBrushCount	= NUM_BRUSH_TYPE;
	ImpBrush::c_pBrushes	= new ImpBrush* [ImpBrush::c_nBrushCount];

	ImpBrush::c_pBrushes[BRUSH_POINTS]	= new PointBrush( this, "Points" );

	// Note: You should implement these 5 brushes.  They are set the same (PointBrush) for now
	ImpBrush::c_pBrushes[BRUSH_LINES]				
		= new LineBrush( this, "Lines" );
	ImpBrush::c_pBrushes[BRUSH_CIRCLES]				
		= new CircleBrush( this, "Circles" );
	ImpBrush::c_pBrushes[BRUSH_SCATTERED_POINTS]	
		= new ScatteredPointsBrush( this, "Scattered Points" );
	ImpBrush::c_pBrushes[BRUSH_SCATTERED_LINES]		
		= new ScatterLinesBrush( this, "Scattered Lines" );
	ImpBrush::c_pBrushes[BRUSH_SCATTERED_CIRCLES]	
		= new ScatteredCirclesBrush( this, "Scattered Circles" );
	ImpBrush::c_pBrushes[BRUSH_HIGHLIGHTER]
		= new Highlighter(this, "Highlighter");

	// make one of the brushes current
	m_pCurrentBrush	= ImpBrush::c_pBrushes[0];

	m_pCurrentBrushDirection = 0;
}


//---------------------------------------------------------
// Set the current UI 
//---------------------------------------------------------
void ImpressionistDoc::setUI(ImpressionistUI* ui) 
{
	m_pUI	= ui;
}

//---------------------------------------------------------
// Returns the active picture/painting name
//---------------------------------------------------------
char* ImpressionistDoc::getImageName() 
{
	return m_imageName;
}

//---------------------------------------------------------
// Called by the UI when the user changes the brush type.
// type: one of the defined brush types.
//---------------------------------------------------------
void ImpressionistDoc::setBrushType(int type)
{
	m_pCurrentBrush	= ImpBrush::c_pBrushes[type];
}

//---------------------------------------------------------
// Returns the size of the brush.
//---------------------------------------------------------
int ImpressionistDoc::getSize()
{
	return m_pUI->getSize();
}

int ImpressionistDoc::getWidth()
{
	return m_pUI->getWidth();
}

int ImpressionistDoc::getAngle()
{
	if (m_pCurrentBrushDirection==0)
		return m_pUI->getAngle();
	else if (m_pCurrentBrushDirection == 1) {
		return gradientAngle;
	}
	else if (m_pCurrentBrushDirection == 2) {
		return movementAngle;
	}
}

int ImpressionistDoc::getBrushDirection()
{
	return m_pCurrentBrushDirection;
}

float ImpressionistDoc::getAlpha()
{
	return m_pUI->getAlpha();
}

void ImpressionistDoc::setBrushDirection(int type)
{
	m_pCurrentBrushDirection = type;
}

void ImpressionistDoc::updateBrushDirection(const Point source, const Point target, bool start)
{
	if (m_pCurrentBrushDirection == 0)
		return;
	else if (m_pCurrentBrushDirection == 1) {//Gradient
		setGradientDirection(target);
	}
	else if (m_pCurrentBrushDirection == 2) {
		setMovementDirection(target, start);
	}
}

void ImpressionistDoc::setMovementDirection(const Point target, bool start) {
	float pi = 3.14159265;
	if (start) {
		movementAngle=0;
	}
	else {
		movementAngle = atan2((previousPoint.y - target.y), (previousPoint.x - target.x)) * 180 / pi;
	}
	//printf("movementAngle:%f, previous point: (%f,%f), current point: (%f,%f)\n", movementAngle, previousPoint.x, previousPoint.y, target.x, target.y);
	previousPoint = target;
}

void ImpressionistDoc::setGradientDirection(const Point source) {
	float pi = 3.141592654;
	int sobelX[3][3] = { { -1,0,1 },{ -2,0,2 } ,{ -1,0,1 } };
	int sobelY[3][3] = { { -1,-2,-1 },{ 0,0,0 } ,{ 1,2,1 } };
	GLfloat grayscale[3][3];
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j <  3; j++) {
			GLubyte color[3] = {0,0,0};
			memcpy(color, GetOriginalPixel(source.x + i-1, source.y+j-1),3);
			grayscale[i][j] = (static_cast<GLfloat>(color[0])/255 + static_cast<GLfloat>(color[1])/255 + static_cast<GLfloat>(color[2])/255) / 3;
			//printf("grayscale: %f, color: (%f,%f,%f)\n", grayscale[i][j], static_cast<GLfloat>(color[0]), static_cast<GLfloat>(color[1]), static_cast<GLfloat>(color[2]));
		}
	}

	float sobelXValue = 0;
	float sobelYValue = 0;
	// apply sobel kernel
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			sobelXValue += grayscale[i][j] * sobelX[i][j];
			sobelYValue += grayscale[i][j] * sobelY[i][j];
		}
	}
	gradientAngle = atan2(sobelYValue, sobelXValue) * 180 / pi;
	//printf("gradientAngle: %f, sobelValue: (%f,%f)\n", gradientAngle, sobelXValue, sobelYValue);
}

//---------------------------------------------------------
// Load the specified image
// This is called by the UI when the load image button is 
// pressed.
//---------------------------------------------------------
int ImpressionistDoc::loadImage(char *iname) 
{
	// try to open the image to read
	unsigned char*	data;
	int				width, 
					height;

	if ( (data=readBMP(iname, width, height))==NULL ) 
	{
		fl_alert("Can't load bitmap file");
		return 0;
	}

	// reflect the fact of loading the new image
	m_nWidth		= width;
	m_nPaintWidth	= width;
	m_nHeight		= height;
	m_nPaintHeight	= height;

	// release old storage
	if ( m_ucBitmap ) delete [] m_ucBitmap;
	if ( m_ucPainting ) delete [] m_ucPainting;

	m_ucBitmap		= data;

	// allocate space for draw view
	m_ucPainting	= new unsigned char [width*height*3];
	memset(m_ucPainting, 0, width*height*3);

	m_pUI->m_mainWindow->resize(m_pUI->m_mainWindow->x(), 
								m_pUI->m_mainWindow->y(), 
								width*2, 
								height+25);

	// display it on origView
	m_pUI->m_origView->resizeWindow(width, height);	
	m_pUI->m_origView->refresh();

	// refresh paint view as well
	m_pUI->m_paintView->resizeWindow(width, height);	
	m_pUI->m_paintView->refresh();


	return 1;
}


//----------------------------------------------------------------
// Save the specified image
// This is called by the UI when the save image menu button is 
// pressed.
//----------------------------------------------------------------
int ImpressionistDoc::saveImage(char *iname) 
{

	writeBMP(iname, m_nPaintWidth, m_nPaintHeight, m_ucPainting);

	return 1;
}

//----------------------------------------------------------------
// Clear the drawing canvas
// This is called by the UI when the clear canvas menu item is 
// chosen
//-----------------------------------------------------------------
int ImpressionistDoc::clearCanvas() 
{

	// Release old storage
	if ( m_ucPainting ) 
	{
		delete [] m_ucPainting;

		// allocate space for draw view
		m_ucPainting	= new unsigned char [m_nPaintWidth*m_nPaintHeight*3];
		memset(m_ucPainting, 0, m_nPaintWidth*m_nPaintHeight*3);

		// refresh paint view as well	
		m_pUI->m_paintView->refresh();
	}
	
	return 0;
}

//------------------------------------------------------------------
// Get the color of the pixel in the original image at coord x and y
//------------------------------------------------------------------
GLubyte* ImpressionistDoc::GetOriginalPixel( int x, int y )
{
	if ( x < 0 ) 
		x = 0;
	else if ( x >= m_nWidth ) 
		x = m_nWidth-1;

	if ( y < 0 ) 
		y = 0;
	else if ( y >= m_nHeight ) 
		y = m_nHeight-1;

	return (GLubyte*)(m_ucBitmap + 3 * (y*m_nWidth + x));
}

//----------------------------------------------------------------
// Get the color of the pixel in the original image at point p
//----------------------------------------------------------------
GLubyte* ImpressionistDoc::GetOriginalPixel( const Point p )
{
	return GetOriginalPixel( p.x, p.y );
}

