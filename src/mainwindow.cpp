#include "mainwindow.h"
#include "settings.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QSettings>
#include <QLabel>
#include <QGroupBox>
#include <iostream>

void MainWindow::initialize() {
    realtime = new Realtime;
    aspectRatioWidget = new AspectRatioWidget(this);
    aspectRatioWidget->setAspectWidget(realtime, 3.f/4.f);
    QHBoxLayout *hLayout = new QHBoxLayout; // horizontal alignment
    QVBoxLayout *vLayout = new QVBoxLayout(); // vertical alignment
    vLayout->setAlignment(Qt::AlignTop);
    hLayout->addLayout(vLayout);
    hLayout->addWidget(aspectRatioWidget, 1);
    this->setLayout(hLayout);

    // Create labels in sidebox
    QFont font;
    font.setPointSize(12);
    font.setBold(true);
    QLabel *tesselation_label = new QLabel(); // Parameters label
    tesselation_label->setText("Tesselation");
    tesselation_label->setFont(font);
    QLabel *camera_label = new QLabel(); // Camera label
    camera_label->setText("Camera");
    camera_label->setFont(font);
    QLabel *fog_label = new QLabel(); // Fog label
    fog_label->setText("Fog");
    fog_label->setFont(font);
    QLabel *exposure_label = new QLabel(); // Exposure label
    exposure_label->setText("Exposure");
    exposure_label->setFont(font);

    // From old Project 6
    // QLabel *filters_label = new QLabel(); // Filters label
    // filters_label->setText("Filters");
    // filters_label->setFont(font);

    QLabel *ec_label = new QLabel(); // Extra Credit label
    ec_label->setText("Extra Credit");
    ec_label->setFont(font);
    QLabel *param1_label = new QLabel(); // Parameter 1 label
    param1_label->setText("Parameter 1:");
    QLabel *param2_label = new QLabel(); // Parameter 2 label
    param2_label->setText("Parameter 2:");
    QLabel *near_label = new QLabel(); // Near plane label
    near_label->setText("Near Plane:");
    QLabel *far_label = new QLabel(); // Far plane label
    far_label->setText("Far Plane:");
    QLabel *fogMin_label = new QLabel(); // Fog min distance label
    fogMin_label->setText("Fog Min Distance:");
    QLabel *fogMax_label = new QLabel(); // Fog max distance label
    fogMax_label->setText("Fog Max Distance:");


    // From old Project 6
    // // Create checkbox for per-pixel filter
    // filter1 = new QCheckBox();
    // filter1->setText(QStringLiteral("Per-Pixel Filter"));
    // filter1->setChecked(false);
    // // Create checkbox for kernel-based filter
    // filter2 = new QCheckBox();
    // filter2->setText(QStringLiteral("Kernel-Based Filter"));
    // filter2->setChecked(false);

    // Create file uploader for scene file
    uploadFile = new QPushButton();
    uploadFile->setText(QStringLiteral("Upload Scene File"));
    
    saveImage = new QPushButton();
    saveImage->setText(QStringLiteral("Save Image"));

    // Creates the boxes containing the parameter sliders and number boxes
    QGroupBox *p1Layout = new QGroupBox(); // horizonal slider 1 alignment
    QHBoxLayout *l1 = new QHBoxLayout();
    QGroupBox *p2Layout = new QGroupBox(); // horizonal slider 2 alignment
    QHBoxLayout *l2 = new QHBoxLayout();

    // Create slider controls to control parameters
    p1Slider = new QSlider(Qt::Orientation::Horizontal); // Parameter 1 slider
    p1Slider->setTickInterval(1);
    p1Slider->setMinimum(1);
    p1Slider->setMaximum(25);
    p1Slider->setValue(1);

    p1Box = new QSpinBox();
    p1Box->setMinimum(1);
    p1Box->setMaximum(25);
    p1Box->setSingleStep(1);
    p1Box->setValue(1);

    p2Slider = new QSlider(Qt::Orientation::Horizontal); // Parameter 2 slider
    p2Slider->setTickInterval(1);
    p2Slider->setMinimum(1);
    p2Slider->setMaximum(25);
    p2Slider->setValue(1);

    p2Box = new QSpinBox();
    p2Box->setMinimum(1);
    p2Box->setMaximum(25);
    p2Box->setSingleStep(1);
    p2Box->setValue(1);

    // Adds the slider and number box to the parameter layouts
    l1->addWidget(p1Slider);
    l1->addWidget(p1Box);
    p1Layout->setLayout(l1);

    l2->addWidget(p2Slider);
    l2->addWidget(p2Box);
    p2Layout->setLayout(l2);

    // Creates the boxes containing the camera sliders and number boxes
    QGroupBox *nearLayout = new QGroupBox(); // horizonal near slider alignment
    QHBoxLayout *lnear = new QHBoxLayout();
    QGroupBox *farLayout = new QGroupBox(); // horizonal far slider alignment
    QHBoxLayout *lfar = new QHBoxLayout();

    // Create slider controls to control near/far planes
    nearSlider = new QSlider(Qt::Orientation::Horizontal); // Near plane slider
    nearSlider->setTickInterval(1);
    nearSlider->setMinimum(1);
    nearSlider->setMaximum(1000);
    nearSlider->setValue(10);

    nearBox = new QDoubleSpinBox();
    nearBox->setMinimum(0.01f);
    nearBox->setMaximum(10.f);
    nearBox->setSingleStep(0.1f);
    nearBox->setValue(0.1f);

    farSlider = new QSlider(Qt::Orientation::Horizontal); // Far plane slider
    farSlider->setTickInterval(1);
    farSlider->setMinimum(1000);
    farSlider->setMaximum(10000);
    farSlider->setValue(10000);

    farBox = new QDoubleSpinBox();
    farBox->setMinimum(10.f);
    farBox->setMaximum(100.f);
    farBox->setSingleStep(0.1f);
    farBox->setValue(100.f);

    // Adds the slider and number box to the parameter layouts
    lnear->addWidget(nearSlider);
    lnear->addWidget(nearBox);
    nearLayout->setLayout(lnear);

    lfar->addWidget(farSlider);
    lfar->addWidget(farBox);
    farLayout->setLayout(lfar);

    // Creates the boxes containing the fog sliders and number boxes
    QGroupBox *fogMinLayout = new QGroupBox();
    QHBoxLayout *lfogMin = new QHBoxLayout();

    QGroupBox *fogMaxLayout = new QGroupBox();
    QHBoxLayout *lfogMax = new QHBoxLayout();

    // Create sliders
    fogMinSlider = new QSlider(Qt::Horizontal);
    fogMinSlider->setTickInterval(1);
    fogMinSlider->setMinimum(0);
    fogMinSlider->setMaximum(10000);
    fogMinSlider->setValue(0);

    fogMinBox = new QDoubleSpinBox();
    fogMinBox->setMinimum(0.0);
    fogMinBox->setMaximum(100.0);
    fogMinBox->setSingleStep(0.1);
    fogMinBox->setValue(0.0);

    fogMaxSlider = new QSlider(Qt::Horizontal);
    fogMaxSlider->setTickInterval(1);
    fogMaxSlider->setMinimum(0);
    fogMaxSlider->setMaximum(10000);
    fogMaxSlider->setValue(0);

    fogMaxBox = new QDoubleSpinBox();
    fogMaxBox->setMinimum(0.0);
    fogMaxBox->setMaximum(100.0);
    fogMaxBox->setSingleStep(0.1);
    fogMaxBox->setValue(0.0);

    // Adds the slider and number box to the parameter layouts
    lfogMin->addWidget(fogMinSlider);
    lfogMin->addWidget(fogMinBox);
    fogMinLayout->setLayout(lfogMin);

    lfogMax->addWidget(fogMaxSlider);
    lfogMax->addWidget(fogMaxBox);
    fogMaxLayout->setLayout(lfogMax);

    // Creates box containing the exposure slider and number box
    QGroupBox *exposureLayout = new QGroupBox();
    QHBoxLayout *lexposure = new QHBoxLayout();

    // Create sliders
    exposureSlider = new QSlider(Qt::Orientation::Horizontal);
    exposureSlider->setTickInterval(1);
    exposureSlider->setMinimum(0);
    exposureSlider->setMaximum(1000);
    exposureSlider->setValue(100);

    exposureBox = new QDoubleSpinBox();
    exposureBox->setDecimals(5);
    exposureBox->setMinimum(0.0);
    exposureBox->setMaximum(10.0);
    exposureBox->setSingleStep(0.01);
    exposureBox->setValue(1.0);

    // Adds the slider and number box to the parameter layouts
    lexposure->addWidget(exposureSlider);
    lexposure->addWidget(exposureBox);
    exposureLayout->setLayout(lexposure);

    // Extra Credit:
    ec1 = new QCheckBox();
    ec1->setText(QStringLiteral("Bloom"));
    ec1->setChecked(false);

    ec2 = new QCheckBox();
    ec2->setText(QStringLiteral("Color Grade"));
    ec2->setChecked(false);

    ec3 = new QCheckBox();
    ec3->setText(QStringLiteral("Extra Credit 3"));
    ec3->setChecked(false);

    ec4 = new QCheckBox();
    ec4->setText(QStringLiteral("Extra Credit 4"));
    ec4->setChecked(false);

    vLayout->addWidget(uploadFile);
    vLayout->addWidget(saveImage);
    vLayout->addWidget(tesselation_label);
    vLayout->addWidget(param1_label);
    vLayout->addWidget(p1Layout);
    vLayout->addWidget(param2_label);
    vLayout->addWidget(p2Layout);
    vLayout->addWidget(camera_label);
    vLayout->addWidget(near_label);
    vLayout->addWidget(nearLayout);
    vLayout->addWidget(far_label);
    vLayout->addWidget(farLayout);

    vLayout->addWidget(fog_label);
    vLayout->addWidget(fogMin_label);
    vLayout->addWidget(fogMinLayout);
    vLayout->addWidget(fogMax_label);
    vLayout->addWidget(fogMaxLayout);

    vLayout->addWidget(exposure_label);
     vLayout->addWidget(exposureLayout);

    // From old Project 6
    // vLayout->addWidget(filters_label);
    // vLayout->addWidget(filter1);
    // vLayout->addWidget(filter2);

    // Extra Credit:
    vLayout->addWidget(ec_label);
    vLayout->addWidget(ec1);
    vLayout->addWidget(ec2);
    vLayout->addWidget(ec3);
    vLayout->addWidget(ec4);

    connectUIElements();

    // Set default values of 5 for tesselation parameters
    onValChangeP1(5);
    onValChangeP2(5);

    // Set default values for near and far planes
    onValChangeNearBox(0.1f);
    //onValChangeFarBox(10.f);
    onValChangeFarBox(100.f);
}

void MainWindow::finish() {
    realtime->finish();
    delete(realtime);
}

void MainWindow::connectUIElements() {
    // From old Project 6
    //connectPerPixelFilter();
    //connectKernelBasedFilter();
    connectUploadFile();
    connectSaveImage();
    connectParam1();
    connectParam2();
    connectNear();
    connectFar();
    connectFog();
    connectExposure();
    connectExtraCredit();
}


// From old Project 6
// void MainWindow::connectPerPixelFilter() {
//     connect(filter1, &QCheckBox::clicked, this, &MainWindow::onPerPixelFilter);
// }
// void MainWindow::connectKernelBasedFilter() {
//     connect(filter2, &QCheckBox::clicked, this, &MainWindow::onKernelBasedFilter);
// }

void MainWindow::connectUploadFile() {
    connect(uploadFile, &QPushButton::clicked, this, &MainWindow::onUploadFile);
}

void MainWindow::connectSaveImage() {
    connect(saveImage, &QPushButton::clicked, this, &MainWindow::onSaveImage);
}

void MainWindow::connectParam1() {
    connect(p1Slider, &QSlider::valueChanged, this, &MainWindow::onValChangeP1);
    connect(p1Box, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MainWindow::onValChangeP1);
}

void MainWindow::connectParam2() {
    connect(p2Slider, &QSlider::valueChanged, this, &MainWindow::onValChangeP2);
    connect(p2Box, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MainWindow::onValChangeP2);
}

void MainWindow::connectNear() {
    connect(nearSlider, &QSlider::valueChanged, this, &MainWindow::onValChangeNearSlider);
    connect(nearBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangeNearBox);
}

void MainWindow::connectFar() {
    connect(farSlider, &QSlider::valueChanged, this, &MainWindow::onValChangeFarSlider);
    connect(farBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangeFarBox);
}

void MainWindow::connectFog() {
    connect(fogMinSlider, &QSlider::valueChanged,
            this, &MainWindow::onValChangeFogMinSlider);
    connect(fogMinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangeFogMinBox);

    connect(fogMaxSlider, &QSlider::valueChanged,
            this, &MainWindow::onValChangeFogMaxSlider);
    connect(fogMaxBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangeFogMaxBox);
}

void MainWindow::connectExposure() {
    connect(exposureSlider, &QSlider::valueChanged, this, &MainWindow::onValChangeExposure);
    connect(exposureBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double d) {
                exposureSlider->setValue(int(d * 100.0));
                settings.exposure = d;
                realtime->settingsChanged();
            });
}

void MainWindow::connectExtraCredit() {
    connect(ec1, &QCheckBox::clicked, this, &MainWindow::onBloom);
     connect(ec2, &QCheckBox::clicked, this, &MainWindow::onGraded);
    connect(ec3, &QCheckBox::clicked, this, &MainWindow::onExtraCredit3);
    connect(ec4, &QCheckBox::clicked, this, &MainWindow::onExtraCredit4);
}

// From old Project 6
// void MainWindow::onPerPixelFilter() {
//     settings.perPixelFilter = !settings.perPixelFilter;
//     realtime->settingsChanged();
// }
// void MainWindow::onKernelBasedFilter() {
//     settings.kernelBasedFilter = !settings.kernelBasedFilter;
//     realtime->settingsChanged();
// }

void MainWindow::onUploadFile() {
    // Get abs path of scene file
    QString configFilePath = QFileDialog::getOpenFileName(this, tr("Upload File"),
                                                          QDir::currentPath()
                                                              .append(QDir::separator())
                                                              .append("scenefiles")
                                                              .append(QDir::separator())
                                                              .append("realtime")
                                                              .append(QDir::separator())
                                                              .append("required"), tr("Scene Files (*.json)"));
    if (configFilePath.isNull()) {
        std::cout << "Failed to load null scenefile." << std::endl;
        return;
    }

    settings.sceneFilePath = configFilePath.toStdString();

    std::cout << "Loaded scenefile: \"" << configFilePath.toStdString() << "\"." << std::endl;

    realtime->sceneChanged();
}

void MainWindow::onSaveImage() {
    if (settings.sceneFilePath.empty()) {
        std::cout << "No scene file loaded." << std::endl;
        return;
    }
    std::string sceneName = settings.sceneFilePath.substr(0, settings.sceneFilePath.find_last_of("."));
    sceneName = sceneName.substr(sceneName.find_last_of("/")+1);
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Image"),
                                                    QDir::currentPath()
                                                        .append(QDir::separator())
                                                        .append("student_outputs")
                                                        .append(QDir::separator())
                                                        .append("realtime")
                                                        .append(QDir::separator())
                                                        .append("required")
                                                        .append(QDir::separator())
                                                        .append(sceneName), tr("Image Files (*.png)"));
    std::cout << "Saving image to: \"" << filePath.toStdString() << "\"." << std::endl;
    realtime->saveViewportImage(filePath.toStdString());
}

void MainWindow::onValChangeP1(int newValue) {
    p1Slider->setValue(newValue);
    p1Box->setValue(newValue);
    settings.shapeParameter1 = p1Slider->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeP2(int newValue) {
    p2Slider->setValue(newValue);
    p2Box->setValue(newValue);
    settings.shapeParameter2 = p2Slider->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeNearSlider(int newValue) {
    //nearSlider->setValue(newValue);
    nearBox->setValue(newValue/100.f);
    settings.nearPlane = nearBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFarSlider(int newValue) {
    //farSlider->setValue(newValue);
    farBox->setValue(newValue/100.f);
    settings.farPlane = farBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeNearBox(double newValue) {
    nearSlider->setValue(int(newValue*100.f));
    //nearBox->setValue(newValue);
    settings.nearPlane = nearBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFarBox(double newValue) {
    farSlider->setValue(int(newValue*100.f));
    //farBox->setValue(newValue);
    settings.farPlane = farBox->value();
    realtime->settingsChanged();
}

// Fog
void MainWindow::onValChangeFogMinSlider(int newValue) {
    fogMinBox->setValue(newValue / 100.f);
    settings.fogMin = fogMinBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFogMinBox(double newValue) {
    fogMinSlider->setValue(int(newValue * 100.f));
    settings.fogMin = fogMinBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFogMaxSlider(int newValue) {
    fogMaxBox->setValue(newValue / 100.f);
    settings.fogMax = fogMaxBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFogMaxBox(double newValue) {
    fogMaxSlider->setValue(int(newValue * 100.f));
    settings.fogMax = fogMaxBox->value();
    realtime->settingsChanged();
}

// Exposure
void MainWindow::onValChangeExposure(int newValue) {
    float realExposure = newValue / 100.0f;
    exposureSlider->setValue(newValue);
    exposureBox->setValue(realExposure);
    settings.exposure = realExposure;
    realtime->settingsChanged();
}

// Extra Credit:

void MainWindow::onBloom() {
    settings.bloom = !settings.bloom;
    realtime->settingsChanged();
}

void MainWindow::onGraded() {
    settings.graded = !settings.graded;
    realtime->settingsChanged();
}

void MainWindow::onExtraCredit3() {
    settings.extraCredit3 = !settings.extraCredit3;
    realtime->settingsChanged();
}

void MainWindow::onExtraCredit4() {
    settings.extraCredit4 = !settings.extraCredit4;
    realtime->settingsChanged();
}
