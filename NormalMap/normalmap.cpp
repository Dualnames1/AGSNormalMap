////*

#ifdef WIN32
#define WINDOWS_VERSION
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning(disable : 4244)
#endif

#if !defined(BUILTIN_PLUGINS)
#define THIS_IS_THE_PLUGIN
#endif

#include <math.h>


#if defined(PSP_VERSION)
#include <pspsdk.h>
#include <pspmath.h>
#include <pspdisplay.h>
#define sin(x) vfpu_sinf(x)
#endif

#include "plugin/agsplugin.h"

#if defined(BUILTIN_PLUGINS)
namespace agsnormalmap {
#endif

#if defined(__GNUC__)
inline unsigned long _blender_alpha16_bgr(unsigned long y) __attribute__((always_inline));
inline void calc_x_n(unsigned long bla) __attribute__((always_inline));
#endif

const unsigned int Magic = 0xACAB0000;
const unsigned int Version = 1;
const unsigned int SaveMagic = Magic + Version;
//const float PI = 3.14159265f;

int screen_width = 640;
int screen_height = 360;
int screen_color_depth = 32;

IAGSEngine* engine;



// Imported script functions

int getRcolorFromInt(int color) {
	return ((color >> 16) & 0xFF);
}
int getGcolorFromInt(int color) {
	return ((color >> 8) & 0xFF);
}
int getBcolorFromInt(int color) {
	return ((color >> 0) & 0xFF);
}
int getAcolorFromInt(int color)
{
	return ((color >> 24) & 0xFF);
}
float clamp(float x, float min, float max) {
    float value=x;
	if(value < min) value= min;
    if(value > max) value= max;
    return value;
}

int ConvertColorToGrayScale(int color)
{
	int r=getRcolorFromInt(color);
	int g=getGcolorFromInt(color);
	int b=getBcolorFromInt(color);
	int a=getAcolorFromInt(color);

	float d=float ((r * r + g * g + b * b) /3);
	int gr = int(sqrt(d));

	return ((gr << 16) | (gr << 8) | (gr << 0) | (a << 24));
}



struct NormalMap
{
	float nmapR[230400];//(screen_width*screen_height) - 640/360
	float nmapG[230400];
	float nmapB[230400];
	int txtuR[230400];
	int txtuG[230400];
	int txtuB[230400];
	float shiny;
	float specularity;
	int Ownerx;
	int Ownery;
	int sprite;//Not the normal map!!
	int width;
	int height;
};

struct Light
{
	int x;
	int y;
	int z;
	int size;
	bool HasChanged;
	float RedTint;
	float GreenTint;
	float BlueTint;
	bool active;
	int luminance;
	
};

int MAX_LIGHTS=20;
struct ColorDta
{
	int holdR[230400];
	int holdG[230400];
	int holdB[230400];
};
ColorDta ColorData[20];//max dif lights

Light Lights[20];
NormalMap NormalMaps[100];//SIZE OF OUR TOTAL NORMAL MAPS


//CREATES NORMAL MAP
void DrawNormalMap(int dsprite,float strength,float value)
{	
	BITMAP* src = engine->GetSpriteGraphic(dsprite);//DEFINE WHERE WE CREATE THE NORMAL MAP (meaning what entity it is)
	unsigned int** pixel = (unsigned int**)engine->GetRawBitmapSurface(src);
	int src_width,src_height, src_depth;
	engine->GetBitmapDimensions (src, &src_width, &src_height, &src_depth);


	strength=clamp(strength,0.0,1.0);
	value=clamp(value,0.0,1.0);

	if (value >0.5) value=1.0;
	if (value <0.5) value=0.0;
	
	if (strength >0.5) strength=1.0;
	if (strength <0.5) strength=0.0;
    

	
	
	int x=0;
	while(x < src_width)
	{
		int y=0;
		while (y<src_height)
		{
			int color = pixel[y][x];
			int a=getAcolorFromInt(color);

			if (a <255)
			{
				//if transparent ignore the pixel.
			}
			else 
			{
				int tx=x-1;
				if (tx <0) tx=0;
				float xLeft = float(ConvertColorToGrayScale(pixel[y][tx]))*strength;

				tx=x+1;
				if (tx >src_width-1) tx=src_width-1;
                float xRight = float(ConvertColorToGrayScale(pixel[y][tx]))*strength;

				int ty=y-1;
				if (ty <0) ty=0;
                float yUp = float(ConvertColorToGrayScale(pixel[ty][x]))*strength;
				
				ty=y+1;
				if (ty >src_height-1) ty=src_height-1;

                float yDown = float(ConvertColorToGrayScale(pixel[ty][x]))*strength;


                float xDelta = ((xLeft-xRight)+1.0)*value;
                float yDelta = ((yUp-yDown)+1.0)*value;
				int red =int(xDelta*255.0);
				int green =int(yDelta*255.0);
				int blue =int(1.0*255.0);
				pixel[y][x] = ((red << 16) | (green << 8) | (blue << 0) | (255 << 24));//yD instead of 255
			}
			


			y++;
		}
		x++;
	}
	engine->ReleaseBitmapSurface(src);
           
}
//CREATES NORMAL MAP


int GetNormalFromSpr(int sprite)
{
	int n=0;
	int id=-1;
	while (n < 100)
	{
		if (NormalMaps[n].sprite!=NULL && NormalMaps[n].sprite==sprite)
		{
			id=n;
		}
		n++;
	}
	return id;
}

void CreateNM(int normalmp,int asprite,int id, float spec,float shine)//LOADS NORMAL MAP TO MEMORY.
{
	int i=0;
	float nx;
	float ny;
	float nz;

	NormalMaps[id].sprite=asprite;
	NormalMaps[id].specularity = spec;
	NormalMaps[id].shiny = shine;


	BITMAP* normalmap = engine->GetSpriteGraphic(normalmp);
	unsigned int** pixel_normal = (unsigned int**)engine->GetRawBitmapSurface(normalmap);
	int normalmap_width,normalmap_height, normalmap_depth;
	engine->GetBitmapDimensions (normalmap, &normalmap_width, &normalmap_height, &normalmap_depth);

	BITMAP* actualsprite = engine->GetSpriteGraphic(asprite);//CHANGE TO ALLOW USER
	unsigned int** pixel_actualsprite = (unsigned int**)engine->GetRawBitmapSurface(actualsprite);

	
	
	int nlength=normalmap_width*normalmap_height;
	NormalMaps[id].width=normalmap_width;
	NormalMaps[id].height=normalmap_height;

	
	
	while(i<normalmap_width)
	{
		int j=0;
		while (j < normalmap_height)
		{
		int px=(i);
		int py=(j);

		int cpix=pixel_normal[py][px];
		int ctxturpix=pixel_actualsprite[py][px];

		nx=float(getRcolorFromInt(cpix));
		ny=float(255-getGcolorFromInt(cpix));
		nz=float(getBcolorFromInt(cpix));		
		float magInv = 1.0/sqrt(nx*nx + ny*ny + nz*nz);
		nx=nx*magInv;
		ny=ny*magInv;
		nz=nz*magInv;
		int d=(i*normalmap_height);
		NormalMaps[id].nmapR[d+j]=nx;
		NormalMaps[id].txtuR[d+j]=getRcolorFromInt(ctxturpix);
		NormalMaps[id].nmapG[d+j]=ny;
		NormalMaps[id].txtuG[d+j]=getGcolorFromInt(ctxturpix);
		NormalMaps[id].nmapB[d+j]=nz;
		NormalMaps[id].txtuB[d+j]=getBcolorFromInt(ctxturpix);

		j++;
		}

		i++;
	}


	engine->ReleaseBitmapSurface(actualsprite);
	engine->ReleaseBitmapSurface(normalmap);

}


bool HasLightUpdated(int id,int lx,int ly,int lz)
{
	if (Lights[id].x==lx && Lights[id].y==ly && Lights[id].size==lz)
	{
		Lights[id].HasChanged=false;
	}
	else 
	{
		Lights[id].HasChanged=true;
		Lights[id].x=lx;
		Lights[id].y=ly;
		Lights[id].size=lz;
	}
	return Lights[id].HasChanged;
}

void SetLightRad(int id, int radius)
{
	Lights[id].size=radius;
}

int GetLightRad(int id)
{
	return Lights[id].size;
}

void SetLightSta(int id, bool state)
{
	Lights[id].active=state;
}

bool GetLightSta(int id)
{
	return Lights[id].active;
}
int GetLightX(int id)
{
	return Lights[id].x;
}
int GetLightY(int id)
{
	return Lights[id].y;
}

void SetLightZ(int id,int value)
{
	Lights[id].z=value;
}
int GetLightZ(int id)
{
	return Lights[id].z;
}


void SetLightClr(int id, int red, int green, int blue,int luminance)
{
	
	Lights[id].RedTint=clamp(float(red),0.0,255.0);
	Lights[id].GreenTint=clamp(float(green),0.0,255.0);
	Lights[id].BlueTint=clamp(float(blue),0.0,255.0);
	Lights[id].luminance=int (clamp(float(luminance),0.0,255.0));
}

void SetNMSpec(int id, float specularity)
{
	NormalMaps[id].specularity=specularity;
}
void SetNMSh(int id, float shine)
{
	NormalMaps[id].shiny=shine;
}

void SetLight(int id,int x,int y,int radius,bool state)
{
	Lights[id].x=x;
	Lights[id].y=y;
	Lights[id].size=radius;
	Lights[id].active=state;
	
}

void SetNMXY(int id, int OwnerX,int OwnerY)
{
	if (OwnerX!=NormalMaps[id].Ownerx || OwnerY!=NormalMaps[id].Ownery)
	{
		int lid=0;//LIGHT ID
		while (lid < MAX_LIGHTS)
		{
			if (Lights[lid].active)
			{
				Lights[lid].HasChanged=true;
			}
			lid++;
		}
	}
	NormalMaps[id].Ownerx=OwnerX;
	NormalMaps[id].Ownery=OwnerY;	
}

int GetColorDark(int x,int y,int id,float by,int height)
{	
	int j=(x*height)+y;
	int r=NormalMaps[id].txtuR[j];
	int g=NormalMaps[id].txtuG[j];
	int b=NormalMaps[id].txtuB[j];
	r=int(clamp(float(r)/by, 0.0, 255.0));
	g=int(clamp(float(g)/by, 0.0, 255.0));
	b=int(clamp(float(b)/by, 0.0, 255.0));
	int a=255;
	unsigned int colorInt= ((r << 16) | (g << 8) | (b << 0) | (a << 24));
	return colorInt;
}

bool IsPixelTransparent(int xx,int yy, int nid)
{
	int j=(xx)+yy;
	int rd=NormalMaps[nid].txtuR[j];
	int gd=NormalMaps[nid].txtuG[j];
	int bd=NormalMaps[nid].txtuB[j];
	if (rd >0 && gd >0 && bd>0)
	{
		return false;
	}
	else return true;
}




void UpdateNM(int spriteG,int id)
{	
	BITMAP* src = engine->GetSpriteGraphic(spriteG);
	unsigned int** pixel_src = (unsigned int**)engine->GetRawBitmapSurface(src);
	
	int src_width;
	int src_height;
	int src_depth;
	engine->GetBitmapDimensions (src, &src_width, &src_height, &src_depth);


   // src_width=int(0.5*src_width);
//	src_height=int(0.5*src_height);
	//LIGHTS
	//x,y,size  lx,ly,lz
    
	

	//CHECK IF LIGHT IS CLOSE TO NORMAL MAP OWNER
	int lid=0;//LIGHT ID
	bool firstlight=false;
	

	int fiD=0;
	int clr=0;
	while (lid < MAX_LIGHTS)
	{
		if (Lights[lid].active)
		{
			int lx=Lights[lid].x;
			int ly=Lights[lid].y;
			int zaxis=Lights[lid].z;
			int lz=Lights[lid].size;
			int onX=NormalMaps[id].Ownerx;//-Lights[lid].size;
			int onY=NormalMaps[id].Ownery;
			int lum = Lights[lid].luminance;
			
			
			int x,y;
			float nx;
			float ny;
			float nz;
			float na;
			float dx;
			float dy;
			float dz;
			float doublelz=lz*2.0;

			//float zval=abs(zaxis);
			//zval=clamp(zval, 0.0,50.0);

			//int maxx=0;

			float difLX=float(lx-onX);
			float difLY=float(ly-onY);

			float additive=0.0;

			

			//if (zaxis >200)//<0)
			//{
				/*
				//int getColor=pixel_src[y][x];
				int yy=0;
				for (yy = 0; yy < src_height; yy++)
				{
					int xx=0;	
					for (xx = 0; xx < src_width; xx++)
					{

						int j=(xx*src_height)+yy;
						int r=NormalMaps[id].txtuR[j];
						int g=NormalMaps[id].txtuG[j];
						int b=NormalMaps[id].txtuB[j];

						if (r >0 && g >0 && b>0)
						{
							pixel_src[yy][xx]=GetColorDark(xx,yy,id,darkness,src_height);
						}
					}
				}



				for (y = 0; y < src_height; y++)
				{
					x=0;	
					for (x = 0; x < src_width; x++)
					{

					int j=(x*src_height)+y;
					int r=NormalMaps[id].txtuR[j];
					int g=NormalMaps[id].txtuG[j];
					int b=NormalMaps[id].txtuB[j];

					if (r >0 && g >0 && b>0)
					{
						int k=0;
						while (k < thick)
						{
							int ax=x-k;
							if (ax <0) ax=0;
							if (ax > src_width) ax=src_width;

							int jx=(ax*src_height)+yy;
							int rx=Lights[lid].RedTint;
							int gx=Lights[lid].GreenTint;
							int bx=Lights[lid].BlueTint;
							
							pixel_src[y][ax]=((rx << 16) | (gx << 8) | (bx << 0) | ((255-int(zval*5.0)) << 24));
								
							k++;
						}

						int bx=x+1;
						int by=y;

						int grabID=-1;

						while (bx < src_width)
						{
							if (bx <0) bx=0;
							if (bx > src_width) bx=src_width;
							int grabColor=pixel_src[y][bx];

							int getj=(bx*src_height)+by;
							int r=NormalMaps[id].txtuR[getj];
							int g=NormalMaps[id].txtuG[getj];
							int b=NormalMaps[id].txtuB[getj];

							if (r<=0 || g<=0 || b<=0)
							{
								grabID=bx;
								int k=0;
								while (k < thick)
								{
									int ax=bx-1;
									if (ax <0) ax=0;
									if (ax > src_width) ax=src_width;

									int jx=((ax+k)*src_height)+yy;
									int rx=Lights[lid].RedTint;
									int gx=Lights[lid].GreenTint;
									int bx=Lights[lid].BlueTint;
									pixel_src[y][ax+k]=((rx << 16) | (gx << 8) | (bx << 0) | ( (255-int(zval*5.0)) << 24));
									k++;
								}
								bx=src_width;
							}


							bx++;
						}

						if (grabID==-1)	x=src_width;
						else x= grabID;
					}
*/

/*
					int ni=(x*src_height)+y;
					int r=NormalMaps[id].txtuR[ni];
					int g=NormalMaps[id].txtuG[ni];
					int b=NormalMaps[id].txtuB[ni];
					if (r > 0 && g > 0 && b > 0)
					{
						//zval=1.0;
						
						//the smaller the Z axis
						//the closer the light is to the back, thus 'bigger' darkness
						//ADJUST DARKNESS BY POSITION OF SAID LIGHT
						//x and y
						r=int(clamp(float(r)/darkness, 0.0, 255.0));
						g=int(clamp(float(g)/darkness, 0.0, 255.0));
						b=int(clamp(float(b)/darkness, 0.0, 255.0));
						//if difference from center > x
						float d_x= x - (src_width/2);
						float d_y= y - (src_height/2);
						//d_x=d_x*d_x;
						float grabDif=sqrt((d_x*d_x) + (d_y*d_y));
						int a=255;//allow it to grasp values from file
						unsigned int colorInt= ((r << 16) | (g << 8) | (b << 0) | (a << 24));
						pixel_src[y][x]=colorInt;
						str-=4;
					}*/

					//}
				//}
				//return;
			//}
			//else 
			//{

			//0-40 600-640
			for (x = 0; x < src_width; x++)
			{				
				y=0;
				for (y = 0; y < src_height; y++)
				{

					int ni=(x*src_height)+y;
					nx=NormalMaps[id].nmapR[ni];
					ny=NormalMaps[id].nmapG[ni];
					nz=NormalMaps[id].nmapB[ni];
					if ((ni&1)==0)
					{
						dx = difLX - x;
						dy = difLY- y;
						dz = float(lz);
						// normalize it
						float magInv = 1.0/sqrt(dx*dx + dy*dy + dz*dz);
						dx = dx*magInv;
						dy = dy*magInv; 
						dz = dz*magInv;						
					}
					float dot = dx*nx + dy*ny + dz*nz;
					float spec =  pow(dot,static_cast<float>(20.0)) * NormalMaps[id].specularity;
					spec = spec+  pow(dot, static_cast<float>(400.0))*NormalMaps[id].shiny;
					float intensity = spec + 0.5;

					int setR=NormalMaps[id].txtuR[ni];
					int setG=NormalMaps[id].txtuG[ni];
					int setB=NormalMaps[id].txtuB[ni];

					if((setR>lum && setG>lum && setB>lum) || zaxis <0)
					{						
						float difx= difLX - x;
						float dify= difLY - y;
						difx=sqrt((difx*difx) + (dify*dify));
						float adjust=(1.0-(difx+1.0)/doublelz)*2.0;

						if (difx < doublelz) //&& zaxis >=0) //RESTORE after figuring out how to push pixel outwards
						{
							
							//adjust=adjust*2.0;
							if (intensity>1.0)
							{
								setR+=int(Lights[lid].RedTint*adjust);
								setG+=int(Lights[lid].GreenTint*adjust);
								setB+=int(Lights[lid].BlueTint*adjust);
							}
							else 
							{
								setR+=int(Lights[lid].RedTint*adjust);
								setG+=int(Lights[lid].GreenTint*adjust);
								setB+=int(Lights[lid].BlueTint*adjust);
							}
						}
						int r=int(clamp(float(setR)*intensity, 0.0, 255.0));
						int g=int(clamp(float(setG)*intensity, 0.0, 255.0));
						int b=int(clamp( (float(setB)*intensity), 0.0, 255.0));

						
						
						//if (zaxis<0)
						//{
							//if (r <=0) r=0;
						//}
						
						//GET COLOR FROM RGB to INT
						int a=255;//allow it to grasp values from file
						unsigned int colorInt= ((r << 16) | (g << 8) | (b << 0) | (a << 24));
						int colorwhite=((255 << 16) | (255 << 8) | (255 << 0) | (a << 24));


						if (zaxis <0)
						{
							int j=(x*src_height)+y;
							int rd=NormalMaps[id].txtuR[j];
							int gd=NormalMaps[id].txtuG[j];
							int bd=NormalMaps[id].txtuB[j];


							if (!IsPixelTransparent(x*src_height,y,id))
							{
								/*
								//NON TRANSPARENT PIXELS								
								if (rd > 0 && gd > 0 && bd > 0)
								{
									float darkness=1.0+additive;
									rd=int(clamp(float(rd)/darkness, 0.0, 255.0));
									gd=int(clamp(float(gd)/darkness, 0.0, 255.0));
									bd=int(clamp(float(bd)/darkness, 0.0, 255.0));
									//if difference from center > x
									//float d_x= x - (src_width/2);
									//float d_y= y - (src_height/2);
									//d_x=d_x*d_x;
									//float grabDif=sqrt((d_x*d_x) + (d_y*d_y));
									int a=255;//allow it to grasp values from file
									unsigned int colorInt= ((rd << 16) | (gd << 8) | (bd << 0) | (a << 24));
									pixel_src[y][x]=colorInt;
								}*/
								//NON TRANSPARENT PIXELS
							}
							else
							{
								int gx=-1;
								int tcount=0;
								while (gx <2)
								{
									int gy=-1;
									while (gy < 2)
									{
										int sX=x+gx;
										int sY=y+gy;

										if (x+gx<0) sX=0;
										if (x+gx>src_width) sX=src_width;

										if (y+gy<0) sY=0;
										if (y+gy>src_height) sY=src_height;


										if (!IsPixelTransparent((x+gx)*src_height,y+gy,id))
										{
											tcount++;
										}

										gy++;
									}
									gx++;
								}
								if (tcount>=2)
								{
									bool pixelLeftTransparent=IsPixelTransparent((x-1)*src_height,y,id);
									bool pixelRightTransparent=IsPixelTransparent((x+1)*src_height,y,id);

									if (pixelLeftTransparent && !pixelRightTransparent && x-1>0)
									{
										int dn=((x+1)*src_height)+y;
										int setRx=NormalMaps[id].txtuR[dn];
										int setGx=NormalMaps[id].txtuG[dn];
										int setBx=NormalMaps[id].txtuB[dn];
										setRx+=int(Lights[lid].RedTint*adjust);
										setGx+=int(Lights[lid].GreenTint*adjust);
										setBx+=int(Lights[lid].BlueTint*adjust);
										int rx=int(clamp(float(setRx)*intensity, 0.0, 255.0));
										int gx=int(clamp(float(setGx)*intensity, 0.0, 255.0));
										int bx=int(clamp( (float(setBx)*intensity), 0.0, 255.0));
										int colorLeft=((rx << 16) | (gx << 8) | (bx << 0) | (255 << 24));

										if (rx >4 && gx > 4 && bx>4)
										{
											pixel_src[y][x]=colorLeft;//colorInt;
											additive+=1.0;
										}
									}
									else if (pixelRightTransparent && !pixelLeftTransparent && x+1<=src_width)
									{
										int dn=((x-1)*src_height)+y;
										int setRx=NormalMaps[id].txtuR[dn];
										int setGx=NormalMaps[id].txtuG[dn];
										int setBx=NormalMaps[id].txtuB[dn];
										setRx+=int(Lights[lid].RedTint*adjust);
										setGx+=int(Lights[lid].GreenTint*adjust);
										setBx+=int(Lights[lid].BlueTint*adjust);
										int rx=int(clamp(float(setRx)*intensity, 0.0, 255.0));
										int gx=int(clamp(float(setGx)*intensity, 0.0, 255.0));
										int bx=int(clamp(float(setBx)*intensity, 0.0, 255.0));
										int colorLeft=((rx << 16) | (gx << 8) | (bx << 0) | (255 << 24));
										if (rx >4 && gx > 4 && bx>4)
										{
											pixel_src[y][x]=colorLeft;
											additive+=1.0;
										}
									}
								}


							}
						}
						else 
						{
							if (firstlight)
							{
								r=(ColorData[clr-1].holdR[fiD]+1+r)/2;
								g=(ColorData[clr-1].holdG[fiD]+1+g)/2;
								b=(ColorData[clr-1].holdB[fiD]+1+b)/2;
								fiD++;
								r=clamp(float(r),0.0,255.0);
								g=clamp(float(g),0.0,255.0);
								b=clamp(float(b),0.0,255.0);
								colorInt= ((r << 16) | (g << 8) | (b << 0) | (a << 24));
								pixel_src [y][x] = colorInt;
								ColorData[clr].holdR[fiD-1]=r;
								ColorData[clr].holdG[fiD-1]=g;
								ColorData[clr].holdB[fiD-1]=b;
							}
							else 
							{
								ColorData[clr].holdR[fiD]=r;
								ColorData[clr].holdG[fiD]=g;
								ColorData[clr].holdB[fiD]=b;
								fiD++;
								pixel_src [y][x] = colorInt;	
							}
						}
					}
					}					
				
			}
			fiD=0;
			clr+=1;
			firstlight=true;
			//}

        if (zaxis <0)
		{
			int xb=0;
			while (xb < src_width)
			{
				int yb=0;
				while (yb < src_height)
				{
					if (!IsPixelTransparent(xb*src_height,yb,id))
					{
						int j=(xb*src_height)+yb;
						int rd=NormalMaps[id].txtuR[j];
						int gd=NormalMaps[id].txtuG[j];
						int bd=NormalMaps[id].txtuB[j];
						//NON TRANSPARENT PIXELS
						if (rd > 0 && gd > 0 && bd > 0)
						{
							//additive=clamp(additive,0.0,20.0);
							float darkness=1.0+(10.0/10.0);
							rd=int(clamp(float(rd)/darkness, 0.0, 255.0));
							gd=int(clamp(float(gd)/darkness, 0.0, 255.0));
							bd=int(clamp(float(bd)/darkness, 0.0, 255.0));
							int a=255;//allow it to grasp values from file
							unsigned int colorInt= ((rd << 16) | (gd << 8) | (bd << 0) | (a << 24));
							pixel_src[yb][xb]=colorInt;
						}
					}
					yb++;
				}
				xb++;
			}
		}


			//////
		}
		

		lid++;
		
	}
	engine->ReleaseBitmapSurface(src);
  
}

// ********************************************
// ************  AGS Interface  ***************
// ********************************************




void UpdateNormalMap(int dyn_sprite,int id)
{
	UpdateNM(dyn_sprite,id);
}
void CreateNormalMap(int normalmap,int actsprite,int id,float spec,float shine)
{
	CreateNM(normalmap,actsprite,id,spec,shine);
}
void CreateLight(int id,int x,int y,int radius,bool state)
{
	SetLight(id,x,y,radius,state);
}

void SetNormalMapXY(int id, int x,int y)
{
	SetNMXY(id, x,y);
}


int GetNormalFromSprite(int sprite)
{
	return GetNormalFromSpr(sprite);
}
void SetLightColor(int id, int red, int green, int blue,int luminance)
{
	SetLightClr(id, red, green, blue,luminance);
}


void SetLightState(int id, bool state)
{
	SetLightSta(id, state);
}

bool GetLightState(int id)
{
	return GetLightSta(id);
}


void SetLightRadius(int id, int radius)
{
	SetLightRad(id,radius);
}

int GetLightRadius(int id)
{
	return GetLightRad(id);
}



void SetNormalMapSpecularity(int id, float specularity)
{
	SetNMSpec(id, specularity);
}
void SetNormalMapShine(int id, float shine)
{
	SetNMSh(id,shine);
}

bool IsLightUpdated(int id,int lx,int ly,int size)
{
	return HasLightUpdated(id,lx,ly,size);
}


void AGS_EngineStartup(IAGSEngine *lpEngine)
{
  engine = lpEngine;
  
  if (engine->version < 13) 
    engine->AbortGame("Engine interface is too old, need newer version of AGS.");
  
    engine->RegisterScriptFunction("UpdateNormalMap",(void*)&UpdateNormalMap);
	engine->RegisterScriptFunction("CreateNormalMap",(void*)&CreateNormalMap);
	engine->RegisterScriptFunction("CreateLight",(void*)&CreateLight);
	engine->RegisterScriptFunction("SetNormalMapXY",(void*)&SetNormalMapXY);
	engine->RegisterScriptFunction("GetNormalFromSprite",(void*)&GetNormalFromSprite);
	engine->RegisterScriptFunction("SetLightColor",(void*)&SetLightColor);
	engine->RegisterScriptFunction("SetNormalMapSpecularity",(void*)&SetNormalMapSpecularity);
	engine->RegisterScriptFunction("SetNormalMapShine",(void*)&SetNormalMapShine);
	engine->RegisterScriptFunction("IsLightUpdated",(void*)&IsLightUpdated);
	engine->RegisterScriptFunction("DrawNormalMap",(void*)&DrawNormalMap);    
	engine->RegisterScriptFunction("SetLightRadius",(void*)&SetLightRadius);
	engine->RegisterScriptFunction("GetLightRadius",(void*)&GetLightRadius);	
	engine->RegisterScriptFunction("SetLightState",(void*)&SetLightState);
	engine->RegisterScriptFunction("GetLightState",(void*)&GetLightState);
    engine->RegisterScriptFunction("GetLightX",(void*)&GetLightX);
	engine->RegisterScriptFunction("GetLightY",(void*)&GetLightY);
	engine->RegisterScriptFunction("GetLightZ",(void*)&GetLightZ);
	engine->RegisterScriptFunction("SetLightZ",(void*)&SetLightZ);


	int lid=0;//LIGHT ID
		while (lid < MAX_LIGHTS)
		{
			Lights[lid].z=0;
			lid++;
		}


  /*
  
  
  
  
  engine->RegisterScriptFunction("SetLightSpecularity",(void*)&SetLightSpecularity);
  engine->RegisterScriptFunction("SetLightShine",(void*)&SetLightShine);
*/
  

  //engine->RegisterScriptFunction("",(void*)&);

  engine->RequestEventHook(AGSE_PREGUIDRAW);
  engine->RequestEventHook(AGSE_PRESCREENDRAW);
  engine->RequestEventHook(AGSE_SAVEGAME);
  engine->RequestEventHook(AGSE_RESTOREGAME);
  engine->RequestEventHook(AGSE_POSTSCREENDRAW);

}


void AGS_EngineInitGfx(const char *driverID, void *data)
{
}

void AGS_EngineShutdown()
{
	
}

int AGS_EngineOnEvent(int event, int data)
{
  if (event == AGSE_PREGUIDRAW)
  {
  }  
  else if (event == AGSE_RESTOREGAME)
  {
	  //engine->FRead(&SFX[j].repeat,sizeof(int),data);
  }
  else if (event == AGSE_SAVEGAME)
  {
	 // engine->FWrite(&currentMusic, sizeof(int), data);	  
  }
  else if (event == AGSE_PRESCREENDRAW)
  {
    // Get screen size once here.
    engine->GetScreenDimensions(&screen_width, &screen_height, &screen_color_depth);	
	
  }
  else if (event ==AGSE_POSTSCREENDRAW)
  {
	  
  }
  return 0;
}

int AGS_EngineDebugHook(const char *scriptName, int lineNum, int reserved)
{
  return 0;
}



#if defined(WINDOWS_VERSION) && !defined(BUILTIN_PLUGINS)

// ********************************************
// ***********  Editor Interface  *************
// ********************************************
//AGSFlashlight
const char* scriptHeader =

"import void UpdateNormalMap(int dyn_sprite,int id);\r\n"
"import void CreateNormalMap(int normalmap,int actsprite,int id,float spec,float shine);\r\n"
"import void CreateLight(int id,int x,int y,int radius,bool state);\r\n"
"import void SetNormalMapXY(int id, int x,int y);\r\n"
"import int GetNormalFromSprite(int sprite);\r\n"
"import void SetLightColor(int id, int red, int green, int blue,int luminance);\r\n"
"import void SetNormalMapSpecularity(int id, float specularity);\r\n"
"import void SetNormalMapShine(int id, float shine);\r\n"
"import bool IsLightUpdated(int id,int lx,int ly,int size);\r\n"
"import void DrawNormalMap(int dsprite,float strength,float value);\r\n"
"import void SetLightRadius(int id, int radius);\r\n"
"import int GetLightRadius(int id);\r\n"
"import void SetLightState(int id, bool state);\r\n"
"import bool GetLightState(int id);\r\n"
"import int GetLightX(int id);\r\n"
"import int GetLightY(int id);\r\n"
"import void SetLightZ(int id,int value);\r\n"
"import int GetLightZ(int id);\r\n"



/*  
  "import void SetLightTint(int id, int red, int green, int blue);\r\n"
  "import int GrabIDFromSprite(int sprite);\r\n"
  "import void SetLightSpecularity(int id, float specularity);\r\n"
  "import void SetLightShine(int id, float shine);\r\n"
*/
   ;
//"import ;\r\n"

IAGSEditor* editor;


LPCSTR AGS_GetPluginName(void)
{
  // Return the plugin description
  return "AGSNormalMap";
}

int  AGS_EditorStartup(IAGSEditor* lpEditor)
{
  // User has checked the plugin to use it in their game

  // If it's an earlier version than what we need, abort.
  if (lpEditor->version < 1)
    return -1;

  editor = lpEditor;
  editor->RegisterScriptHeader(scriptHeader);

  // Return 0 to indicate success
  return 0;
}

void AGS_EditorShutdown()
{
  // User has un-checked the plugin from their game
  editor->UnregisterScriptHeader(scriptHeader);
}

void AGS_EditorProperties(HWND parent)
{
  // User has chosen to view the Properties of the plugin
  // We could load up an options dialog or something here instead
  MessageBoxA(parent, "NormalMap", "About", MB_OK | MB_ICONINFORMATION);
}

int AGS_EditorSaveGame(char* buffer, int bufsize)
{
  // We don't want to save any persistent data
  return 0;
}

void AGS_EditorLoadGame(char* buffer, int bufsize)
{
  // Nothing to load for this plugin
}

#endif


#if defined(BUILTIN_PLUGINS)
} // namespace agsnormalmap
#endif
