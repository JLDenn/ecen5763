#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


typedef double FLOAT;

typedef unsigned int UINT32;
typedef unsigned long long int UINT64;
typedef unsigned char UINT8;

#define MAX_DIM 3000		//Maximum pixels in either dimension supported

// PPM Edge Enhancement Code
//
UINT8 header[22];	//Now used only for the dimensions and depth
UINT8 *R;
UINT8 *G;
UINT8 *B;
UINT8 *convR;
UINT8 *convG;
UINT8 *convB;

#define K 4.0

FLOAT PSF[9] = {-K/8.0, -K/8.0, -K/8.0, -K/8.0, K+1.0, -K/8.0, -K/8.0, -K/8.0, -K/8.0};


int main(int argc, char *argv[])
{
    int fdin, fdout, i, j, rc;
    UINT64 microsecs=0, millisecs=0;
    FLOAT temp;
    
    if(argc < 3)
    {
       printf("Usage: sharpen input_file.ppm output_file.ppm\n");
       exit(-1);
    }
    else
    {
        if((fdin = open(argv[1], O_RDONLY, 0644)) < 0)
        {
            printf("Error opening %s\n", argv[1]);
        }
        //else
        //    printf("File opened successfully\n");

        if((fdout = open(argv[2], (O_RDWR | O_CREAT), 0666)) < 0)
        {
            printf("Error opening %s\n", argv[1]);
        }
        //else
        //    printf("Output file=%s opened successfully\n", "sharpen.ppm");
    }


    //printf("Reading header\n");

	//Find the second \n character which will indicate the start of the width value
	int num_n = 0;
	while(num_n < 2){
		if(read(fdin, (void *)header, 1) < 1){
			printf("Error reading header data\n");
			return -1;
		}
		
		if(header[0] == '\n')
			num_n++;
	}
	
	//At this point we can read the width followed by the height (we'll read until the 2nd '\n')
	num_n = 0;
	i = 0;
	while(num_n < 2){
		if(i == sizeof(header)){
			printf("incorrect format of ppm file dimensions");
			return -1;
		}
		
		if(read(fdin, (void *)&header[i], 1) < 1){
			printf("Error reading header data\n");
			return -1;
		}
		
		if(header[i++] == '\n')
			num_n++;
	}
	
	//Here we have the width/height values separated by \n and ending in a \n. We'll change the ending \n to a \0.
	header[i-1] = '\0';
	
	//Then parse out the width and height, perform a sanity check, then allocate buffers we'll use later.
	unsigned w, h, d;
	if(sscanf(header, "%u %u\n%u", &w, &h, &d) != 3){
		printf("incorrect dimension format in the file header\n");
		return -1;
	}
	
	if(w > MAX_DIM || h > MAX_DIM || d > 255){
		printf("dimensions too large: %u x %u (must be less than %i x %i), or depth > 1 byte\n", w,h, MAX_DIM, MAX_DIM);
		return -1;
	}
	
	//Allocate the buffers we'll use
	R = malloc(w * h);
	G = malloc(w * h);
	B = malloc(w * h);
	convR = malloc(w * h);
	convG = malloc(w * h);
	convB = malloc(w * h);


    //printf("header = %s\n", header); 

    // Read RGB data
    for(i=0; i<h*w; i++)
    {
        rc=read(fdin, (void *)&R[i], 1); convR[i]=R[i];
        rc=read(fdin, (void *)&G[i], 1); convG[i]=G[i];
        rc=read(fdin, (void *)&B[i], 1); convB[i]=B[i];
    }

    // Skip first and last row, no neighbors to convolve with
    for(i=1; i<((h)-1); i++)
    {

        // Skip first and last column, no neighbors to convolve with
        for(j=1; j<((w)-1); j++)
        {
            temp=0;
            temp += (PSF[0] * (FLOAT)R[((i-1)*w)+j-1]);
            temp += (PSF[1] * (FLOAT)R[((i-1)*w)+j]);
            temp += (PSF[2] * (FLOAT)R[((i-1)*w)+j+1]);
            temp += (PSF[3] * (FLOAT)R[((i)*w)+j-1]);
            temp += (PSF[4] * (FLOAT)R[((i)*w)+j]);
            temp += (PSF[5] * (FLOAT)R[((i)*w)+j+1]);
            temp += (PSF[6] * (FLOAT)R[((i+1)*w)+j-1]);
            temp += (PSF[7] * (FLOAT)R[((i+1)*w)+j]);
            temp += (PSF[8] * (FLOAT)R[((i+1)*w)+j+1]);
	    if(temp<0.0) temp=0.0;
	    if(temp>255.0) temp=255.0;
	    convR[(i*w)+j]=(UINT8)temp;

            temp=0;
            temp += (PSF[0] * (FLOAT)G[((i-1)*w)+j-1]);
            temp += (PSF[1] * (FLOAT)G[((i-1)*w)+j]);
            temp += (PSF[2] * (FLOAT)G[((i-1)*w)+j+1]);
            temp += (PSF[3] * (FLOAT)G[((i)*w)+j-1]);
            temp += (PSF[4] * (FLOAT)G[((i)*w)+j]);
            temp += (PSF[5] * (FLOAT)G[((i)*w)+j+1]);
            temp += (PSF[6] * (FLOAT)G[((i+1)*w)+j-1]);
            temp += (PSF[7] * (FLOAT)G[((i+1)*w)+j]);
            temp += (PSF[8] * (FLOAT)G[((i+1)*w)+j+1]);
	    if(temp<0.0) temp=0.0;
	    if(temp>255.0) temp=255.0;
	    convG[(i*w)+j]=(UINT8)temp;

            temp=0;
            temp += (PSF[0] * (FLOAT)B[((i-1)*w)+j-1]);
            temp += (PSF[1] * (FLOAT)B[((i-1)*w)+j]);
            temp += (PSF[2] * (FLOAT)B[((i-1)*w)+j+1]);
            temp += (PSF[3] * (FLOAT)B[((i)*w)+j-1]);
            temp += (PSF[4] * (FLOAT)B[((i)*w)+j]);
            temp += (PSF[5] * (FLOAT)B[((i)*w)+j+1]);
            temp += (PSF[6] * (FLOAT)B[((i+1)*w)+j-1]);
            temp += (PSF[7] * (FLOAT)B[((i+1)*w)+j]);
            temp += (PSF[8] * (FLOAT)B[((i+1)*w)+j+1]);
	    if(temp<0.0) temp=0.0;
	    if(temp>255.0) temp=255.0;
	    convB[(i*w)+j]=(UINT8)temp;
        }
    }

	UINT8 hdr[64];
	int len = snprintf(hdr, sizeof(hdr), "P6\n# brightened image using PSF\n%u %u\n255\n", w, h); 
    rc=write(fdout, (void *)hdr, len);

    // Write RGB data
    for(i=0; i<h*w; i++)
    {
        rc=write(fdout, (void *)&convR[i], 1);
        rc=write(fdout, (void *)&convG[i], 1);
        rc=write(fdout, (void *)&convB[i], 1);
    }


    close(fdin);
    close(fdout);
	
	//Free the memory we allocated for the image data.
	free(R); free(G); free(B);
	free(convR); free(convG); free(convB);
 
}
