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

static Mat preview, tmp1, tmp2, tmp3;
static Mat gray;
static Mat canny;

static Mat capture, document;

static Mat image;

static Quadrilateral savedQuad;
static bool isQuad = false;
static int preCounter = 0;

float scale_factor;

#define JAVA_BUFFER_TYPE_JPEG 256
#define JAVA_BUFFER_TYPE_YUV_420_888 35

extern "C"
jobject quadrilateral_to_java(JNIEnv *jenv, Quadrilateral q) {

    jclass quadrilateral_class = jenv->FindClass("si/vicos/pagescan/Quadrilateral");

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
Java_si_vicos_pagescan_MainActivity_nativeBitmap(JNIEnv *jenv, jclass clazz, jobject test) {
    if(document.empty())
        return false;
    return convert_mat_to_bitmap(jenv, test, document);

}

extern "C"
JNIEXPORT jobject JNICALL
Java_si_vicos_pagescan_MainActivity_nativeSize(JNIEnv *jenv, jclass clazz) {
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
JNIEXPORT jboolean JNICALL
Java_si_vicos_pagescan_MainActivity_nativeCapture(JNIEnv *jenv, jclass clazz, jobject jimage) {

    if (!convert_image_to_mat(jenv, capture, jimage))
        return false;

    Quadrilateral result;
    cvtColor(capture, tmp2, COLOR_BGR2GRAY);

    int areaSize = 307200;

    float aspectRatio = (float)capture.rows / capture.cols;
    int x = round(sqrt(areaSize / aspectRatio));
    int y = x * aspectRatio;

    scale_factor = (float)capture.cols / (float)x;
    resize(tmp2, tmp1, Size(x, y));

    if (get_outline(tmp1, tmp2, tmp3, result, false)) {
        result.p1 *= scale_factor;
        result.p2 *= scale_factor;
        result.p3 *= scale_factor;
        result.p4 *= scale_factor;

        result.area *= scale_factor;

        if(!result.p1.y)
            return false;

        warp_document(capture, result, document);

        int docArea = document.cols * document.rows;
        LOGD("Document area: %d\n", docArea);
        if(docArea > 25000000) {
            resize(document, document, Size(document.cols/2, document.rows/2));
        }
        if(document.cols > 500 && document.rows > 500)
            return true;
    }
    document.release();
    return false;
}


extern "C"
JNIEXPORT jobject JNICALL
Java_si_vicos_pagescan_MainActivity_nativePreview(JNIEnv *jenv, jclass clazz, jobject jimage) {
    if (!convert_image_to_mat(jenv, preview, jimage, IMREAD_GRAYSCALE))
        return NULL;

    Quadrilateral result;

    int areaSize = 153600;

    float aspectRatio = (float)preview.rows / preview.cols;
    int x = round(sqrt(areaSize / aspectRatio));
    int y = x * aspectRatio;

    scale_factor = (float)preview.cols / (float)x;
    resize(preview, tmp1, Size(x, y));

    if (get_outline(tmp1, tmp2, tmp3, result, true)) {

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

