#pragma once

#include <QMainWindow>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include "realtime.h"
#include "utils/aspectratiowidget/aspectratiowidget.hpp"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    void initialize();
    void finish();

private:
    void connectUIElements();
    void connectParam1();
    void connectParam2();
    void connectNear();
    void connectFar();
    void connectFog();
    void connectExposure();
    void connectFocalPlane();
    void connectAperture();
    void connectFocalLength();

    // From old Project 6
    // void connectPerPixelFilter();
    // void connectKernelBasedFilter();

    void connectUploadFile();
    void connectSaveImage();
    void connectExtraCredit();

    Realtime *realtime;
    AspectRatioWidget *aspectRatioWidget;

    // From old Project 6
    // QCheckBox *filter1;
    // QCheckBox *filter2;

    QPushButton *uploadFile;
    QPushButton *saveImage;
    QSlider *p1Slider;
    QSlider *p2Slider;
    QSpinBox *p1Box;
    QSpinBox *p2Box;
    QSlider *nearSlider;
    QSlider *farSlider;
    QDoubleSpinBox *nearBox;
    QDoubleSpinBox *farBox;

    // Fog
    QSlider *fogMinSlider;
    QSlider *fogMaxSlider;
    QDoubleSpinBox *fogMinBox;
    QDoubleSpinBox *fogMaxBox;

    // Exposure
    QSlider *exposureSlider;
    QDoubleSpinBox *exposureBox;

    // Depth of Field
    QSlider *focalPlaneSlider;
    QDoubleSpinBox *focalPlaneBox;

    QSlider *apertureSlider;
    QDoubleSpinBox *apertureBox;

    QSlider *focalLengthSlider;
    QDoubleSpinBox *focalLengthBox;

    // Extra Credit:
    QCheckBox *ec1;
    QCheckBox *ec2;
    QCheckBox *ec3;
    QCheckBox *ec4;

private slots:
    // From old Project 6
    // void onPerPixelFilter();
    // void onKernelBasedFilter();

    void onUploadFile();
    void onSaveImage();
    void onValChangeP1(int newValue);
    void onValChangeP2(int newValue);
    void onValChangeNearSlider(int newValue);
    void onValChangeFarSlider(int newValue);
    void onValChangeNearBox(double newValue);
    void onValChangeFarBox(double newValue);
    void onValChangeFogMinSlider(int newValue);
    void onValChangeFogMinBox(double newValue);
    void onValChangeFogMaxSlider(int newValue);
    void onValChangeFogMaxBox(double newValue);
    void onValChangeExposure(int newValue);

    void onValChangeFocalPlaneSlider(int newValue);
    void onValChangeFocalPlaneBox(double newValue);
    void onValChangeApertureSlider(int newValue);
    void onValChangeApertureBox(double newValue);
    void onValChangeFocalLengthSlider(int newValue);
    void onValChangeFocalLengthBox(double newValue);


    // Extra Credit:
    void onBloom();
    void onGraded();
    void onExtraCredit3();
    void onExtraCredit4();
};
