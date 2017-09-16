#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define TRIMTOP 80
#define TRIMBUT 25
#define TRIMLFT 0
#define TRIMRGT 135
#define CELLSIZE 16

#define DEBUG 1
#include <assert.h>

typedef struct _Img {
	char data[ (CELLSIZE-1)*(CELLSIZE-1)*4 ];
	int mean; // 0:open cell 1-8:num cell 9:close cell 10: flaged bom -1: undefined
	struct _Img *next;
} Img;
	

Img *Imgroot = NULL;

int getfirsttate( XImage *buf ){ //At CELLSIZE = 16
	int x,y;
	char *tmp = buf->data;
	for( x = 0; x < buf->width; x++ ){
		int flg[2] = { 0,0 };
		for( y=0; y < buf->height; y++ ){
			if( !(tmp[(y*(buf->width)+x)*4+0] + tmp[(y*(buf->width)+x)*4+1] + tmp[(y*(buf->width)+x)*4+2]) ) flg[y%2]++;
		}
		if( flg[0]+flg[1] >= buf->height/2 ) break;
	}
	if( x == buf->width ) return -1;
	return x;
}

int getfirstyoko( XImage *buf ){ //At CELLSIZE = 16
	int x,y;
	char *tmp = buf->data;
	for( y = 0; y < buf->height; y++ ){
		int flg[2] = {0,0};
		for( x=0; x < buf->width;  x++ ){
			if( !(tmp[(y*(buf->width)+x)*4+0] + tmp[(y*(buf->width)+x)*4+1] + tmp[(y*(buf->width)+x)*4+2]) ) flg[x%2]++;
		}
		if( flg[0]+flg[1] >= buf->width/2 ) break;
	}
	if( y == buf->height ) return -1;
	return y;
}


int search( char *img ){
	int i;
	Img *tmp = Imgroot;
	if( !tmp ){
		if( !(Imgroot = (Img *)malloc( sizeof(Img) )) ) return -1;
		Imgroot->next = NULL;
		Imgroot->mean = -1;
		memcpy( Imgroot->data, img, (CELLSIZE-1)*(CELLSIZE-1)*4 );
		tmp = Imgroot;
		return 0;
	}
	
	for( i = 0; 1;i++ ){
		if( !(memcmp( tmp->data, img,  (CELLSIZE-1)*(CELLSIZE-1)*4 ) ) ) return i;
		if( !(tmp->next) ){
			if( !(tmp->next = (Img *)malloc(sizeof(Img))) ) return -1;
			tmp = tmp->next;
			memcpy( tmp->data, img, (CELLSIZE-1)*(CELLSIZE-1)*4 );
			tmp->next = NULL;
			tmp->mean = -1;
			return i;
		}
		tmp = tmp->next;
	}
}

int transrate( XImage *img, int left, int top, int *dest ){
	int x,y,i,j;
	char buf[(CELLSIZE-1)*(CELLSIZE-1)*4],*tmp;
	
#ifdef DEBUG
	for( x= left; x < img->width; x+=CELLSIZE){
		for( y = top; y < img->height; y++ ){
			img->data[(x+y*img->width)*4+0] = 0x00;
			img->data[(x+y*img->width)*4+1] = 0x00;
			img->data[(x+y*img->width)*4+2] = 0xff;
			img->data[(x+y*img->width)*4+3] = 0x00;
		}
	}
	for( y = top; y < img->height; y+=CELLSIZE){
		for( x = left; x < img->width; x++ ){
			img->data[(x+y*img->width)*4+0] = 0x00;
			img->data[(x+y*img->width)*4+1] = 0x00;
			img->data[(x+y*img->width)*4+2] = 0xff;
			img->data[(x+y*img->width)*4+3] = 0x00;
		}
	}
#endif
	
	for( y = top+1; y+CELLSIZE < img->height; y+=CELLSIZE ){
		for( x = left+1; x+CELLSIZE < img->width; x+=CELLSIZE ){
			//Copy to buffer
			tmp = buf;
			for( j = 0; j < CELLSIZE-1; j++ ){
				for( i = 0; i < CELLSIZE-1; i++ ){
					*tmp++ = img->data[(i+x+(y+j)*img->width)*4+0];
					*tmp++ = img->data[(i+x+(y+j)*img->width)*4+1];
					*tmp++ = img->data[(i+x+(y+j)*img->width)*4+2];
					*tmp++ = img->data[(i+x+(y+j)*img->width)*4+3];
				}
			}
			//Search to list...
			*dest++ = search( buf );
		} 
	}
	return 0;
}

int countx( int *cells, int x, int y, int width, int height, int stat ){
	int tmp;
	
	if( x < 0 || x >= width || y < 0 || y >= height ) return -1;
	if( x == 0 || x == width-1 || y == 0 || y == height-1 ) return -2;
	
	return	(cells[(y-1)*width+x-1]==stat)+(cells[(y-1)*width+x+1]==stat)+(cells[(y-1)*width+x]==stat)+
			(cells[y*width+x-1]==stat)+										(cells[y*width+x+1]==stat)+
			(cells[(y+1)*width+x-1]==stat)+(cells[(y+1)*width+x+1]==stat)+(cells[(y+1)*width+x]==stat);
}

int set_all( int *cells, int x, int y, int width, int height, int stat ){
	
	if( x < 0 || x >= width || y < 0 || y >= height ) return -1;
	if( x == 0 || x == width-1 || y == 0 || y == height-1 ) return -2;
	if( cells[(y-1)*width+x-1] == 10 ) cells[(y-1)*width+x-1] = stat;
	if( cells[(y-1)*width+x+1] == 10 ) cells[(y-1)*width+x+1] = stat;
	if( cells[(y-1)*width+x] == 10 ) cells[(y-1)*width+x] = stat;
	if( cells[(y+1)*width+x-1] == 10 ) cells[(y+1)*width+x-1] = stat;
	if( cells[(y+1)*width+x+1] == 10 ) cells[(y+1)*width+x+1] = stat;
	if( cells[(y+1)*width+x] == 10 ) cells[(y+1)*width+x] = stat;
	if( cells[y*width+x-1] == 10 ) cells[y*width+x-1] = stat;
	if( cells[y*width+x+1] == 10 ) cells[y*width+x+1] = stat;
	return 0;
}


int main( int argc, char **argv ){
	Display *dpy;
	Window root;
	XWindowAttributes attr;
	int screen;
	XImage *buf;
	int x,y,i;
	int left,top;
	int cellx,celly;
	int *cells, stat[13] = {0};
	FILE *fp;
	
	dpy = XOpenDisplay( "" );
	root = DefaultRootWindow( dpy );
	screen = DefaultScreen( dpy );
	XGetWindowAttributes( dpy, root, &attr );
	printf( "# %d,%d\n", attr.width, attr.height );
	
	//Get Screen-shot & Trim that.
	buf = XGetImage( dpy, root, TRIMLFT, TRIMTOP, attr.width-TRIMLFT-TRIMRGT, attr.height-TRIMTOP-TRIMBUT, 0xffffffff, ZPixmap );
	printf( "# %d,%d\n", buf->width, buf->height );
	
	//Get first lines...( maybe need debug about reliability of value )
	left = getfirsttate( buf );
	top = getfirstyoko( buf );
	if( left >= CELLSIZE ) left %= CELLSIZE;
	if( top >= CELLSIZE ) top %= CELLSIZE;
	printf( "# x= %d, y= %d\n", left, top );
	
	//Get cell count and malloc
	cellx = (buf->width-left)/CELLSIZE;
	celly = (buf->height-top)/CELLSIZE;
	cells = (int *)malloc( cellx*celly*sizeof(int) );
	memset( cells, 0, cellx*celly*sizeof(int));
	printf( "#width=%d\n#height=%d\n", cellx,celly );
	
	
	//Load images from file
	if( fp = fopen( "img.dat", "r" ) ){
		long sz,sp = ftell(fp);
		Img *tmp;
		fseek( fp, 0, SEEK_END );
		sz = (ftell(fp)-sp)/sizeof(Img);
		fseek( fp, 0, SEEK_SET );
		if( !(Imgroot = (Img *)malloc( sizeof(Img)*sz )) ) return -1;
		fread( Imgroot , sizeof(Img), sz, fp );
		fclose( fp );
		tmp = Imgroot;
		tmp[--sz].next = NULL;
		for(sz--;sz>=0; sz--) tmp[sz].next = &tmp[sz+1];
	}
	
	
	//Transrate image to cell-value
	if( transrate( buf, left, top, cells ) == -1 ) printf("#error\n");
	
	//Transrate image to mean
	{
		Img *tmp = Imgroot;
		while( tmp != NULL ){
			if( tmp->mean == -1 ){//Create Window
				Window wr=XCreateSimpleWindow( dpy,RootWindow(dpy,0),0,0,200,200,1,BlackPixel(dpy,0),WhitePixel(dpy,0));
				GC gc;
				XImage *imgtmp;
				int flag = 1;
				
				XMapWindow( dpy, wr );
				gc=XCreateGC(dpy,wr,0,0);
				
				imgtmp = XGetImage( dpy, root, 0, 0, CELLSIZE-1, CELLSIZE-1, 0xffffffff, ZPixmap );
				memcpy( imgtmp->data, tmp->data, (CELLSIZE-1)*(CELLSIZE-1)*4 );
				
				XSelectInput( dpy, wr, KeyPressMask | ExposureMask );
				
				XPutImage( dpy, wr, gc, imgtmp, 0,0,20,20,CELLSIZE-1,CELLSIZE-1 );
				XDrawString( dpy, wr, gc, 20, 50, "0-8:Num a:Non-open",18);
				XDrawString( dpy, wr, gc, 20, 70, "9:Bom z:Don't touch", 19 );
				XFlush(dpy);
				
				while( flag ){
					XEvent event;
					
					XNextEvent( dpy, &event );
					switch( event.type ){
						char *keyinput;
						case KeyPress:
							keyinput = XKeysymToString( XKeycodeToKeysym( dpy, event.xkey.keycode, 0 ) );
							if( *keyinput == 'a' ) tmp->mean = 0x0a;//Not opened
							if( *keyinput == 'z' ) tmp->mean = 0x0b;//Don't touch (server waiting)
							if( *keyinput >= 0x30 && *keyinput <= 0x39 ) tmp->mean = *keyinput-0x30;//Number
							if( tmp->mean != -1 ) flag = 0;
							break;
						case Expose:
							XPutImage( dpy, wr, gc, imgtmp, 0,0,20,20,CELLSIZE-1,CELLSIZE-1 );
							XDrawString( dpy, wr, gc, 20, 50, "0-8:Num a:Non-open",18);
							XDrawString( dpy, wr, gc, 20, 70, "9:Bom z:Don't touch", 19 );
							XFlush(dpy);
							break;
							
					}
				}
				XFreeGC( dpy, gc );
				XDestroyWindow( dpy, wr );
			}
			tmp=tmp->next;
		}
	}
	
	//Save to file...
	{
		if( fp = fopen( "img.dat", "w+" ) ){
			Img *imgtmp = Imgroot;
			while( imgtmp ){
				fwrite( imgtmp, sizeof(Img), 1, fp );
				imgtmp = imgtmp->next;
			}
			fclose( fp );
		}
	}
	
	//Transrate imgnum to imgmean
	{
		int *p = cells;
		printf("#");
		for( i = 0; i < cellx * celly; i++ ){
			Img *imgtmp = Imgroot;
			for( x = 0; imgtmp ;x++ ){
				if( p[i] == x ){
					p[i] = imgtmp->mean;
					break;
				}
				imgtmp = imgtmp->next;
			}
			printf("%c",(p[i]==9)?'x':(p[i]?(p[i]+0x30):' '));
			if( !((i+1)%cellx) ) printf("\n#");
		}		
	}
	
	//Get statistics
	for( y = 0; y < celly; y++ ) for( x = 0; x < cellx; x++ ) stat[cells[y*cellx+x]]++;
	for( i = 0; i < 12; i++ ) printf( "# %d : %d\n", i, stat[i] );
	
	//for move
	if( !stat[10] ) printf("# move!\n");
	
	//Thinking
	{
		do{
			i = 0;
			printf( "#" ); 
			for( y = 1; y < celly-1; y++ ){
				for( x = 1; x < cellx-1; x++ ){
					int closed_count=0,bom_count=0,flag=0;
					if( !(cells[y*cellx+x] > 0 && cells[y*cellx+x] < 9) ) continue; // Not number
					if( !(closed_count = countx( cells, x, y, cellx, celly, 10 ))) continue;// no closed cell
					bom_count = countx( cells, x, y, cellx, celly, 9 ) + countx( cells, x, y, cellx, celly, 11 );
					if( cells[y*cellx+x] == bom_count ){
						set_all( cells, x, y, cellx, celly, 12 ); // all closed cells are not bom
						i = 1;
					}else if( cells[y*cellx+x] == bom_count + closed_count ){
						set_all( cells, x, y, cellx, celly, 11 ); // all closed cells are bom
						i = 1;
					}else continue;
				}
			}
		}while( i );
		
	}

	//disission
	{

	}
	
	//show map
	{
		printf( "\n\n\n\n\n#" );
		for( y = 0; y < celly; y++ ){
			for( x = 0; x < cellx; x++ ) putchar( " 12345678x#XO"[cells[y*cellx+x]] );
			printf( "\n#" );
		}
		for( i = 0; i < 13; i++ ) stat[i] = 0;
		for( y = 0; y < celly; y++ ) for( x = 0; x < cellx; x++ ) stat[cells[y*cellx+x]]++;
		for( i = 0; i < 13; i++ ) printf( "# %d : %d\n", i, stat[i] );
		printf( "\n\n\n\n\n" );
	}


	while(0){
		Window wr=XCreateSimpleWindow( dpy, RootWindow(dpy,0),TRIMLFT,TRIMTOP,buf->width,buf->height,1,BlackPixel(dpy,0),WhitePixel(dpy,0));
		XMapWindow( dpy, wr );
		GC gc=XCreateGC(dpy,wr,0,0);
		
		
		XPutImage( dpy, wr, gc, buf, 0, 0, 0, 0, buf->width, buf->height );
		XFlush(dpy);
	}

	XCloseDisplay( dpy );
	return 0;
}
