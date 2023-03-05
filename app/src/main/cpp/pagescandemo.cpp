#include <jni.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <android/log.h>
#include <opencv2/imgcodecs.hpp>

#include "conversions.h"
#include "detector.h"
#include "logging.h"

using namespace std;
using namespace cv;

static Mat preview, tmp1, tmp2, tmp3, tmp4;
static Mat gray;
static Mat canny;

static Mat capture, document, document2;

static Mat image;

static bool useCanny = true;
static bool timerOn = false;
static Quadrilateral savedQuad;
static bool isQuad = false;
static int preCounter = 0;

float scale_factor;

#define JAVA_BUFFER_TYPE_JPEG 256
#define JAVA_BUFFER_TYPE_YUV_420_888 35

extern "C"
jobject quadrilateral_to_java(JNIEnv *jenv, Quadrilateral q) {

    jclass quadrilateral_class = jenv->FindClass("si/vicos/pagescandemo/Quadrilateral");

    jmethodID method_quadrilateral_construct = jenv->GetMethodID(quadrilateral_class, "<init>", "(IIIIIIII)V");

    if (!method_quadrilateral_construct) {
        jclass clazz = jenv->FindClass("java/lang/Exception");
        jenv->ThrowNew(clazz, "Unable to create Quadrilateral class");
        return NULL;
    }

    jobject obj = jenv->NewObject(quadrilateral_class, method_quadrilateral_construct, q.p1.x, q.p1.y, q.p2.x, q.p2.y,
                                  q.p3.x, q.p3.y, q.p4.x, q.p4.y);

    jenv->DeleteLocalRef(quadrilateral_class);

    return obj;

}

extern "C"
JNIEXPORT jboolean JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeBitmap(JNIEnv *jenv, jclass clazz, jobject test) {
    if(document.empty())
        return false;
    return convert_mat_to_bitmap(jenv, test, document);

}

extern "C"
JNIEXPORT jboolean JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeBitmap2(JNIEnv *jenv, jclass clazz, jobject test) {
    if(document2.empty())
        return false;
    return convert_mat_to_bitmap(jenv, test, document2);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeSize(JNIEnv *jenv, jclass clazz) {
    jclass size_class = jenv->FindClass("android/util/Size");

    jmethodID method_size_construct = jenv->GetMethodID(size_class, "<init>", "(II)V");

    if (!method_size_construct) {
        jclass clazz = jenv->FindClass("java/lang/Exception");
        jenv->ThrowNew(clazz, "Unable to create Size class");
        return NULL;
    }

    if (document.empty()) {

        jenv->DeleteLocalRef(size_class);
        return NULL;

    } else {

        jobject obj = jenv->NewObject(size_class, method_size_construct, document.cols, document.rows);

        jenv->DeleteLocalRef(size_class);

        return obj;

    }
}

extern "C"
JNIEXPORT jobject JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeSize2(JNIEnv *jenv, jclass clazz) {
    jclass size_class = jenv->FindClass("android/util/Size");

    jmethodID method_size_construct = jenv->GetMethodID(size_class, "<init>", "(II)V");

    if (!method_size_construct) {
        jclass clazz = jenv->FindClass("java/lang/Exception");
        jenv->ThrowNew(clazz, "Unable to create Size class");
        return NULL;
    }

    if (document2.empty()) {

        jenv->DeleteLocalRef(size_class);
        return NULL;

    } else {

        jobject obj = jenv->NewObject(size_class, method_size_construct, document2.cols, document2.rows);

        jenv->DeleteLocalRef(size_class);

        return obj;

    }
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeCapture(JNIEnv *jenv, jclass clazz, jobject jimage) {

    if (!convert_image_to_mat(jenv, capture, jimage))
        return false;

    Quadrilateral result;

    cvtColor(capture, tmp2, COLOR_BGR2GRAY);

    //tmp1 = capture;
    //get_outline(tmp1, tmp2, tmp3, result);

    int areaSize = 545920;

    float aspectRatio = (float)capture.rows / capture.cols;
    int x = round(sqrt(areaSize / aspectRatio));
    int y = x * aspectRatio;

    scale_factor = (float)capture.cols / (float)x;
    resize(tmp2, tmp1, Size(x, y));

    //float scale_factor = (float) capture.cols / 640.0;
    //resize(tmp2, tmp1, Size(640.0, (int)((float)capture.rows / scale_factor)));

    if (get_outline(tmp1, tmp2, tmp3, result, timerOn, false)) {
        result.p1 *= scale_factor;
        result.p2 *= scale_factor;
        result.p3 *= scale_factor;
        result.p4 *= scale_factor;

        result.area *= scale_factor;
/*
        //document = tmp3;
        result.p1 = {200,7600};
        result.p2 = {200,200};
        result.p3 = {5600,200};
        result.p4 = {5600,7600};
*/
/*
        document = tmp3;
        GaussianBlur(tmp1, tmp1, Size(5, 5), 0);
        int kernel[9] = {1, 1, 0, 1, 1, 1, 0, 1, 1};
        //cv::threshold(tmp1, tmp2, 0, 255, THRESH_OTSU);
        Canny(tmp1, tmp1, 5, 35, 3, true);
        dilate(tmp1, tmp1, Mat(3, 3, CV_8U, kernel));//2, 2, CV_8U, kernel2
        document2 = tmp1;
        */
        warp_document(capture, result, document);
        int docArea = document.cols * document.rows;
        LOGD("Document area: %d\n", docArea);
        if(docArea > 25000000) {
            resize(document, document, Size(document.cols/2, document.rows/2));
        }
        if(document.cols > 200 && document.rows > 200)
            return true;
    }
    document.release();
    return false;
}


extern "C"
JNIEXPORT jobject JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativePreview(JNIEnv *jenv, jclass clazz, jobject jimage) {
    if (!convert_image_to_mat(jenv, preview, jimage, IMREAD_GRAYSCALE))
        return NULL;

    Quadrilateral result;


    //int areaSize = 307200;
    int areaSize = 153600;

    float aspectRatio = (float)preview.rows / preview.cols;
    int x = round(sqrt(areaSize / aspectRatio));
    int y = x * aspectRatio;


    //int temp = x;
    //x = y;
    //y = temp;

    scale_factor = (float)preview.cols / (float)x;


    resize(preview, tmp1, Size(x, y));

    //scale_factor = (float) preview.cols / 640.0;
    //resize(preview, tmp1, Size(640.0, (int)((float)preview.rows / scale_factor)));

    String timeLog = "";
    if (get_outline(tmp1, tmp2, tmp3, result, timerOn)) {

        result.p1 *= scale_factor;
        result.p2 *= scale_factor;
        result.p3 *= scale_factor;
        result.p4 *= scale_factor;

        result.area *= scale_factor;

        result.area = 0;

        return quadrilateral_to_java(jenv, result);

    } else {
        return NULL;
    }
}
extern "C"
JNIEXPORT jboolean JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeIsQuad(JNIEnv *env, jclass clazz) {
    return isQuad;
}
extern "C"
JNIEXPORT jobjectArray JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativePoints(JNIEnv *jenv, jclass clazz) {
    jclass point_class = jenv->FindClass("si/vicos/pagescandemo/MyPoint");
    vector<Point> intersections = getIntersections();

    jmethodID method_point_construct = jenv->GetMethodID(point_class, "<init>", "(II)V");

    if (!method_point_construct) {
        jclass clazz = jenv->FindClass("java/lang/Exception");
        jenv->ThrowNew(clazz, "Unable to create MyPoint class");
        return NULL;
    }


    jobjectArray result = (*jenv).NewObjectArray(intersections.size(), point_class, NULL);
    if (result == NULL)
        return NULL;

    for (int i = 0; i < intersections.size(); ++i) {
        jobject obj = jenv->NewObject(point_class, method_point_construct, (int) (intersections[i].x * scale_factor),  (int) (intersections[i].y * scale_factor));

        jenv->SetObjectArrayElement(result, i, obj);
    }
    return result;

}
extern "C"
JNIEXPORT jint JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeIntersections(JNIEnv *jenv, jclass clazz) {
    return getIntersections().size();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeUseCanny(JNIEnv *jenv, jclass clazz,
                                                       jboolean use_canny) {
    useCanny = use_canny;
}



extern "C"
JNIEXPORT void JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeTimer(JNIEnv *env, jclass clazz, jboolean start) {
    timerOn = start;
}

int avgTime = 0;



extern "C"
JNIEXPORT jint JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeTime(JNIEnv *env, jclass clazz) {
    return getTime();
}


extern "C"
JNIEXPORT void JNICALL
Java_si_vicos_pagescandemo_MainActivity_nativeMask(JNIEnv *env, jclass clazz, jboolean mask) {
    setMask(mask);
}