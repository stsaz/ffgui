/** GUI-winapi: image
2014, Simon Zolin */

typedef struct ffui_image {
	FFUI_CTL;
	uint click_id;
} ffui_image;

FF_EXTERN int ffui_img_create(ffui_image *im, ffui_window *parent);

static inline void ffui_img_set(ffui_image *im, ffui_icon *ico) {
	ffui_send(im->h, STM_SETIMAGE, IMAGE_ICON, ico->h);
}
