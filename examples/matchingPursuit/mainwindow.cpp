//=============================================================================================================
/**
* @file     mainwindow.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>;
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
*           Sebastian Krause <sebastian.krause@tu-ilmenau.de>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Martin Henfling, Daniel Knobl and Sebastian Krause. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of MainWindow class.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/mp/atom.h>
#include <utils/mp/adaptivemp.h>
#include <utils/mp/fixdictmp.h>
#include <disp/plot.h>

#include "math.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editorwindow.h"
#include "ui_editorwindow.h"
#include "formulaeditor.h"
#include "ui_formulaeditor.h"
#include "enhancededitorwindow.h"
#include "ui_enhancededitorwindow.h"
#include "deletemessagebox.h"
#include "ui_deletemessagebox.h"
#include "processdurationmessagebox.h"
#include "ui_processdurationmessagebox.h"
#include "treebaseddictwindow.h"
#include "ui_treebaseddictwindow.h"
#include "settingwindow.h"
#include "ui_settingwindow.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTableWidgetItem>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QMap>
#include <QtConcurrent>

#include "QtGui"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace UTILSLIB;
using namespace DISPLIB;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

bool _is_saved = true;
bool _new_paint = true;
bool _tbv_is_loading = false;
bool _auto_change = false;
bool _was_partialchecked = false;
bool _come_from_sample_count = false;
bool _come_from_from =false;

qreal _from;
qreal _to;
qreal _signal_energy = 0.0;
qreal _signal_maximum = 0.0;
qreal _signal_negative_scale = 0.0;
qreal _max_pos = 0.0;
qreal _max_neg = 0.0;
qreal _sample_rate = 1;

qint32 _max_tbv_header_width = 280;

QString _save_path = "";
QString _file_name = "";
QString _last_open_path = QDir::homePath() + "/" + "Matching-Pursuit-Toolbox";
QString _last_save_path = QDir::homePath() + "/" + "Matching-Pursuit-Toolbox";

QMap<qint32, bool> _select_channel_map;
QMap<qint32, bool> _select_atoms_map;

QList<QColor> _colors;
QList<QColor> _original_colors;
QList<GaborAtom> _adaptive_atom_list;
QList<FixDictAtom> _fix_dict_atom_list;

MatrixXd _datas;
MatrixXd _times;
MatrixXd _signal_matrix(0, 0);
MatrixXd _original_signal_matrix(0, 0);
MatrixXd _atom_sum_matrix(0, 0);
MatrixXd _residuum_matrix(0, 0);
MatrixXd _real_residuum_matrix(0, 0);

QTime _counter_time;//(0,0);
QTimer *_counter_timer = new QTimer();

QThread* mp_Thread;
AdaptiveMp *adaptive_Mp;
FixDictMp *fixDict_Mp ;

Formulaeditor *_formula_editor;
EditorWindow *_editor_window;
Enhancededitorwindow *_enhanced_editor_window;
settingwindow *_setting_window;

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================


//*************************************************************************************************************************************
// constructor
MainWindow::MainWindow(QWidget *parent) :    QMainWindow(parent),    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    callGraphWindow = new GraphWindow();    
    callGraphWindow->setMinimumHeight(140);
    callGraphWindow->setMinimumWidth(500);
    callGraphWindow->setMaximumHeight(400);
    ui->l_Graph->addWidget(callGraphWindow);

    callAtomSumWindow = new AtomSumWindow();
    callAtomSumWindow->setMinimumHeight(140);
    callAtomSumWindow->setMinimumWidth(500);
    callAtomSumWindow->setMaximumHeight(400);
    ui->l_atoms->addWidget(callAtomSumWindow);

    callResidumWindow = new ResiduumWindow();
    callResidumWindow->setMinimumHeight(140);
    callResidumWindow->setMinimumWidth(500);
    callResidumWindow->setMaximumHeight(400);
    ui->l_res->addWidget(callResidumWindow);

    callYAxisWindow = new YAxisWindow();
    callYAxisWindow->setMinimumHeight(22);
    callYAxisWindow->setMinimumWidth(500);
    callYAxisWindow->setMaximumHeight(22);
    ui->l_YAxis->addWidget(callYAxisWindow);

    // set progressbar
    ui->progressBarCalc->setMinimum(0);
    ui->progressBarCalc->setHidden(true);
    ui->splitter->setStretchFactor(1,4);    

    ui->lb_save_file->setHidden(true);
    ui->lb_from->setHidden(true);
    ui->dsb_from->setHidden(true);
    ui->lb_to->setHidden(true);
    ui->dsb_to->setHidden(true);
    ui->lb_samples->setHidden(true);
    ui->sb_sample_count->setHidden(true);
    ui->cb_all_select->setEnabled(false);

    // set result tableview
    ui->tbv_Results->setColumnCount(5);
    ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];scale\n[sec];trans\n[sec];modu\n[Hz];phase\n[rad]").split(";"));
    ui->tbv_Results->setColumnWidth(0,55);
    ui->tbv_Results->setColumnWidth(1,45);
    ui->tbv_Results->setColumnWidth(2,40);
    ui->tbv_Results->setColumnWidth(3,40);
    ui->tbv_Results->setColumnWidth(4,40);    

    this->cb_model = new QStandardItemModel;
    connect(this->cb_model, SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(cb_selection_changed(const QModelIndex&, const QModelIndex&)));
    connect(ui->tbv_Results->model(), SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(tbv_selection_changed(const QModelIndex&, const QModelIndex&)));
    connect(_counter_timer, SIGNAL(timeout()), this, SLOT(on_time_out()));

    qRegisterMetaType<Eigen::MatrixXd>("MatrixXd");
    qRegisterMetaType<Eigen::VectorXd>("VectorXd");
    qRegisterMetaType<adaptive_atom_list>("adaptive_atom_list");
    qRegisterMetaType<fix_dict_atom_list>("fix_dict_atom_list");

    QDir dir(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox");
    if(!dir.exists())dir.mkdir(".");
    fill_dict_combobox();

    QSettings settings;
    move(settings.value("pos", QPoint(200, 200)).toPoint());
    resize(settings.value("size", QSize(1050, 700)).toSize());
    this->restoreState(settings.value("window_state").toByteArray());
    ui->splitter->restoreState(settings.value("splitter_sizes").toByteArray());
    _last_open_path = settings.value("last_open_path", QDir::homePath()+ "/" + "Matching-Pursuit-Toolbox").toString();
    _last_save_path = settings.value("last_save_path", QDir::homePath()+ "/" + "Matching-Pursuit-Toolbox").toString();
}

//*************************************************************************************************************************************

MainWindow::~MainWindow()
{    
    delete ui;
}

//*************************************************************************************************************************************

void MainWindow::closeEvent(QCloseEvent * event)
{
    QSettings settings;
    if(settings.value("show_warnings",true).toBool() && !_is_saved)
    {
        QString text = "Warning, your changes have not been saved.\nTo close this window and discard your changes\nclick OK otherwise click Cancel and save the current changes.";
        DeleteMessageBox *msg_box = new DeleteMessageBox(text, "Warning...", "OK", "Cancel");
        msg_box->setModal(true);
        qint32 result = msg_box->exec();

        if(result == 0)
        {
            msg_box->close();
            event->ignore();
            return;
        }
        msg_box->close();
    }

    if(!this->isMaximized())
    {
        settings.setValue("pos", pos());
        settings.setValue("size", size());
    }
    settings.setValue("splitter_sizes", ui->splitter->saveState());
    settings.setValue("window_state", this->saveState());
    settings.setValue("maximized", this->isMaximized());
    settings.setValue("last_open_path", _last_open_path);
    settings.setValue("last_save_path", _last_save_path);
    event->accept();
}

//*************************************************************************************************************************************

void MainWindow::fill_dict_combobox()
{    
    QDir dir(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox");
    QStringList filterList;
    filterList.append("*.dict");
    QFileInfoList fileList =  dir.entryInfoList(filterList);

    ui->cb_Dicts->clear();
    for(int i = 0; i < fileList.length(); i++)
        ui->cb_Dicts->addItem(QIcon(":/images/icons/DictIcon.png"), fileList.at(i).baseName());
}

//*************************************************************************************************************************************

void MainWindow::open_file()
{     
    QString temp_file_name = QFileDialog::getOpenFileName(this, "Please select signal file.", _last_open_path,"(*.fif *.txt)");
    if(temp_file_name.isNull()) return;

    QStringList string_list = temp_file_name.split('/');
    _last_open_path = "";
    for(qint32 i = 0; i < string_list.length() - 1; i++)
        _last_open_path += string_list.at(i) + '/';
    _file_name = temp_file_name;
     this->cb_model->clear();
    this->cb_items.clear();

    ui->dsb_sample_rate->setEnabled(true);

    QFile file(_file_name);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: disable to open signal file."));
        return;
    }
    file.close();

    if(_file_name.endsWith(".fif", Qt::CaseInsensitive))
    {        
        ui->dsb_from->setValue(47.000f);
        ui->dsb_to->setValue(47.999f);
        _from = 47.000f;
        _to = 47.999f;
        //read_fiff_ave(_file_name);
        read_fiff_file(_file_name);
        ui->lb_from->setHidden(false);
        ui->dsb_from->setHidden(false);
        ui->lb_to->setHidden(false);
        ui->dsb_to->setHidden(false);
        ui->lb_samples->setHidden(false);
        ui->sb_sample_count->setHidden(false);
        //ui->sb_sample_count->setValue((ui->dsb_to->value() - ui->dsb_from->value()) * ui->dsb_sample_rate->value());
    }
    else
    {
        _from = 0;
        _signal_matrix.resize(0,0);
        read_matlab_file(_file_name);
        ui->lb_from->setHidden(true);
        ui->dsb_from->setHidden(true);
        ui->lb_to->setHidden(true);
        ui->dsb_to->setHidden(true);
        ui->lb_samples->setHidden(true);
        ui->sb_sample_count->setHidden(true);
    }

    _original_signal_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols());
    _original_signal_matrix = _signal_matrix;
    ui->tbv_Results->setRowCount(0);

    fill_channel_combobox();

    _atom_sum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    //ui->progressBarCalc->reset();
    //ui->progressBarCalc->setVisible(false);

    _new_paint = true;
    update();   
}

//*************************************************************************************************************************************

void MainWindow::cb_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    Q_UNUSED(bottomRight);
    QStandardItem* cb_item = this->cb_items[topLeft.row()];
    if(topLeft.row() == ui->cb_channels->count() - 1)
    {
        if(cb_item->checkState() == Qt::Checked)
        {
            for(qint32 i = 0; i < ui->cb_channels->count() - 1; i++)
                if(this->cb_items[i]->checkState() == Qt::Unchecked)
                    this->cb_items[i]->setData(Qt::Checked, Qt::CheckStateRole);
        }
        else
        {
            for(qint32 i = 0; i < ui->cb_channels->count() - 1; i++)
                if(this->cb_items[i]->checkState() == Qt::Checked)
                    this->cb_items[i]->setData(Qt::Unchecked, Qt::CheckStateRole);
        }
        return;
    }

    ui->tbv_Results->setRowCount(0);

    if(cb_item->checkState() == Qt::Unchecked)
        _select_channel_map[topLeft.row()] = false;
    else if(cb_item->checkState() == Qt::Checked)
        _select_channel_map[topLeft.row()] = true;

    qint32 size = 0;

    for(qint32 i = 0; i < _original_signal_matrix.cols(); i++)    
        if(_select_channel_map[i] == true)
            size++;


    _signal_matrix.resize(_original_signal_matrix.rows(), size);
    _atom_sum_matrix.resize(_original_signal_matrix.rows(), size);
    _residuum_matrix.resize(_original_signal_matrix.rows(), size);    

    _colors.clear();
    qint32 selected_chn = 0;

    for(qint32 channels = 0; channels < _original_signal_matrix.cols(); channels++)    
        if(_select_channel_map[channels] == true)
        {
            _colors.append(_original_colors.at(channels));
            _signal_matrix.col(selected_chn) = _original_signal_matrix.col(channels);            
            selected_chn++;
        }

    _new_paint = true;
    update();
}

//*************************************************************************************************************************************

void MainWindow::read_fiff_ave(QString file_name)
{
    QList<QIODevice*> t_listSampleFilesIn;
    t_listSampleFilesIn.append(new QFile(file_name));
    FiffIO p_fiffIO(t_listSampleFilesIn);

    //std::cout << p_fiffIO << std::endl;

    //Read raw data samples
    p_fiffIO.m_qlistRaw[0]->read_raw_segment_times(_datas, _times, _from, _to);

    ui->dsb_sample_rate->setValue(600);//raw.info.sfreq);
    ui->dsb_sample_rate->setEnabled(false);
    _sample_rate = 600; //ui->sb_sample_rate->value();

    qint32 cols = 5;
    if(_datas.cols() <= 5)   cols = _datas.cols();
    _signal_matrix.resize(_datas.cols(),cols);

    for(qint32 channels = 0; channels < cols; channels++)
        _signal_matrix.col(channels) = _datas.row(channels);
}

//*************************************************************************************************************************************

qint32 MainWindow::read_fiff_file(QString fileName)
{
    //   Setup for reading the raw data
    QFile t_fileRaw(fileName);
    FiffRawData raw(t_fileRaw);

    //   Set up pick list: MEG + STI 014 - bad channels
    QStringList include;
    include << "STI 014";
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;
    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);

    //   Read a data segment
    //   times output argument is optional
    if (!raw.read_raw_segment_times(_datas, _times, _from + 0.002, _to, picks))
    {
       printf("Could not read raw segment.\n");
       return -1;
    }
    printf("Read %d samples.\n",(qint32)_datas.cols());

    ui->dsb_sample_rate->setValue(raw.info.sfreq);
    ui->dsb_sample_rate->setEnabled(false);
    _sample_rate = raw.info.sfreq;

    //ToDo: read all channels, or only a few?!
    qint32 rows = 305;
    if(_datas.rows() <= rows)   rows = _datas.rows();
    _signal_matrix.resize(_datas.cols(),rows);

    for(qint32 channels = 0; channels < rows; channels++)
        _signal_matrix.col(channels) = _datas.row(channels);

    ui->sb_sample_count->setValue((qint32)_datas.cols());

    return 0;
}

//*************************************************************************************************************************************

void MainWindow::read_fiff_file_new(QString file_name)
{
    this->cb_model->clear();
    this->cb_items.clear();

    read_fiff_file(file_name);
    _original_signal_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols());
    _original_signal_matrix = _signal_matrix;

    fill_channel_combobox();
    ui->tbv_Results->setRowCount(0);
    _atom_sum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    _new_paint = true;
    update();
}

//*************************************************************************************************************************************

void MainWindow::fill_channel_combobox()
{
    _colors.clear();
    _colors.append(QColor(0, 0, 0));
    for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
    {
        _colors.append(QColor::fromHsv(qrand() % 256, 255, 190));
        //channel item
        this->cb_item = new QStandardItem;
        this->cb_item->setText(QString("channel %1").arg(channels));
        this->cb_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        this->cb_item->setData(Qt::Checked, Qt::CheckStateRole);
        this->cb_model->insertRow(channels, this->cb_item);
        this->cb_items.push_back(this->cb_item);
        _select_channel_map.insert(channels, true);
    }
     _original_colors = _colors;
    //select all channels item
    this->cb_item = new QStandardItem;
    this->cb_item->setText("de/select all channels");
    this->cb_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    this->cb_item->setData(Qt::Checked, Qt::CheckStateRole);
    this->cb_model->appendRow(this->cb_item);
    this->cb_items.push_back(this->cb_item);

    ui->cb_channels->setModel(this->cb_model);
}

//*************************************************************************************************************************************

void MainWindow::read_matlab_file(QString fileName)
{
    QFile file(fileName);
    QString contents;
    QList<QString> strList;
    file.open(QIODevice::ReadOnly);
    while(!file.atEnd())
    {
        strList.append(file.readLine(0).constData());
    }
    int rowNumber = 0;
    _signal_matrix.resize(strList.length(), 1);
    file.close();
    file.open(QIODevice::ReadOnly);
    while(!file.atEnd())
    {
        contents = file.readLine(0).constData();

        bool isFloat;
        qreal value = contents.toFloat(&isFloat);
        if(!isFloat)
        {
            QString errorSignal = QString("The signal could not completly read. Line %1 from file %2 coud not be readed.").arg(rowNumber).arg(fileName);
            QMessageBox::warning(this, tr("error"),
            errorSignal);
            return;
        }
        _signal_matrix(rowNumber, 0) = value;
        rowNumber++;
    }


    file.close();
    _signal_energy = 0;
    for(qint32 i = 0; i < _signal_matrix.rows(); i++)
        _signal_energy += (_signal_matrix(i, 0) * _signal_matrix(i, 0));
}

//*************************************************************************************************************************************

void GraphWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    paint_signal(_signal_matrix, this->size());
}

//*************************************************************************************************************************************

void GraphWindow::paint_signal(MatrixXd signalMatrix, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));     // paint window white

    if(signalMatrix.rows() > 0 && signalMatrix.cols() > 0)
    {
        qint32 maxStrLenght = 55;           // max lenght in pixel of x-axis string
        qint32 borderMarginWidth = 5;       // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginHeigth = 15;     // reduce paintspace in GraphWindow of borderMargin pixels
        qreal maxPos = 0.0;                 // highest signalvalue
        qreal maxNeg = 0.0;                 // smalest signalvalue
        qreal maxmax = 0.0;                 // absolute difference maxpos - maxneg

        qreal scaleYText = 0.0;
        qint32 negScale  = 0;
        if(_new_paint)
        {
            maxPos = signalMatrix.maxCoeff();
            maxNeg = signalMatrix.minCoeff();

            // absolute signalheight to globe
            if(maxNeg <= 0) maxmax = maxPos - maxNeg;
            else  maxmax = maxPos + maxNeg;

            _max_pos = maxPos;          // to globe max_pos
            _max_neg = maxNeg;          // to globe min_pos
            _signal_maximum = maxmax;   // to globe abs_max
            _new_paint = false;
        }
        // scale axis title
        scaleYText = _signal_maximum / 10.0;
        negScale =  floor((_max_neg * 10 / _signal_maximum) + 0.5);
        _signal_negative_scale = negScale;  // to globe _signal_negative_scale
        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20) borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)signalMatrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / _signal_maximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        for(qint32 i = 1; i <= 11; i++)
        {
            if(negScale == 0)                                                       // x-Axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)   // over all Channels
                {
                    QPolygonF poly;
                    for(qint32 h =0; h < signalMatrix.rows(); h++)
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((signalMatrix(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                    QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawPolyline(poly);
                }
                // paint x-axis
                for(qint32 j = 1; j < 22; j++)
                {
                    if(fmod(j, 4.0) == 0)
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - windowSize.height())), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + windowSize.height())));   // scalelines
                    }
                    QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // scalelines
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }
            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, QString::number(negScale * scaleYText, 'g', 3));     // paint scalevalue y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
            negScale++;
        }
        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis       
    }
    painter.end();    
}

//*************************************************************************************************************************************

void AtomSumWindow::paintEvent(QPaintEvent* event)
{
     Q_UNUSED(event);
    paint_atom_sum(_atom_sum_matrix, this->size(), _signal_maximum, _signal_negative_scale);
}

//*************************************************************************************************************************************

void AtomSumWindow::paint_atom_sum(MatrixXd atom_matrix, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum)
{
    // paint window white
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

    // can also checked of zerovector, then you paint no empty axis
    if(atom_matrix.rows() > 0 && atom_matrix.cols() > 0  && _signal_matrix.rows() > 0 && _signal_matrix.cols() > 0)
    {
        qint32 maxStrLenght = 55;
        qint32 borderMarginHeigth = 15;                     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;                       // reduce paintspace in GraphWindow of borderMargin pixels

        // scale axis title
        qreal scaleYText = (qreal)signalMaximum / (qreal)10;

         while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth)) / (qreal)atom_matrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)signalMaximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;     

        for(qint32 i = 1; i <= 11; i++)
        {
            if(signalNegativeMaximum == 0)                                          // x-Axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < atom_matrix.cols(); channel++)    // over all Channels
                {
                    QPolygonF poly;                   
                    for(qint32 h = 0; h < atom_matrix.rows(); h++)
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((atom_matrix(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                    QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawPolyline(poly);
                }                
                // paint x-axis                
                for(qint32 j = 1; j < 22; j++)
                {
                    if(fmod(j, 4.0) == 0)
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - windowSize.height())), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + windowSize.height())));   // scalelines
                    }
                    QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // scalelines
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, QString::number(signalNegativeMaximum * scaleYText, 'g', 3));     // paint scalvalue Y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
            signalNegativeMaximum++;
        }
        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis      
    }
    painter.end();
}

//*************************************************************************************************************************************

void ResiduumWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    paint_residuum(_residuum_matrix, this->size(), _signal_maximum, _signal_negative_scale);
}

//*************************************************************************************************************************************

void ResiduumWindow::paint_residuum(MatrixXd residuum_matrix, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum)
{
    // paint window white
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

    if(residuum_matrix.rows() > 0 && residuum_matrix.cols() > 0 && _signal_matrix.rows() > 0 && _signal_matrix.cols() > 0)
    {
        qint32 maxStrLenght = 55;
        qint32 borderMarginHeigth = 15;                 // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;                   // reduce paintspace in GraphWindow of borderMargin pixels

        // scale axis title
        qreal scaleYText = (qreal)signalMaximum / (qreal)10;

        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)residuum_matrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)signalMaximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        for(qint32 i = 1; i <= 11; i++)
        {
            if(signalNegativeMaximum == 0)                                              // x-axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < residuum_matrix.cols(); channel++)    // over all Channels
                {
                    QPolygonF poly;                    
                    for(qint32 h = 0; h < residuum_matrix.rows(); h++)
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((residuum_matrix(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                    QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawPolyline(poly);
                }
                // paint x-axis
                for(qint32 j = 1; j < 22; j++)
                {
                    if(fmod(j, 4.0) == 0)
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - windowSize.height())), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + windowSize.height())));   // scalelines
                    }                   
                    QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // scalelines
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, QString::number(signalNegativeMaximum * scaleYText, 'g', 3));     // paint scalevalue y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
            signalNegativeMaximum++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);       // paint y-axis
    }
    painter.end();
}

//*************************************************************************************************************************************

void YAxisWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    paint_axis(_signal_matrix, this->size());
}

//*************************************************************************************************************************************

void YAxisWindow::paint_axis(MatrixXd signalMatrix, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

    if(signalMatrix.rows() > 0 && signalMatrix.cols() > 0)
    {
        qint32 borderMarginWidth = 15;
        while((windowSize.width() - 55 - borderMarginWidth) % 20)borderMarginWidth++;
        qreal scaleXText = (qreal)signalMatrix.rows() /  (qreal)_sample_rate / (qreal)20;     // divide signallegnth
        qreal scaleXAchse = (qreal)(windowSize.width() - 55 - borderMarginWidth) / (qreal)20;

        qint32 j = 0;
        while(j <= 21)
        {
            QString str;
            painter.drawText(j * scaleXAchse + 45, 20, str.append(QString::number(j * scaleXText + _from, 'f', 2)));  // scalevalue as string
            painter.drawLine(j * scaleXAchse + 55, 5 + 2, j * scaleXAchse + 55 , 5 - 2);                            // scalelines
            j++;
        }
        painter.drawText(5 , 20, "[sec]");  // unit
        painter.drawLine(5, 5, windowSize.width()-5, 5);  // paint y-line
    }
}

//*************************************************************************************************************************************

// starts MP-algorithm
void MainWindow::on_btt_Calc_clicked()
{
    TruncationCriterion criterion;

    if(ui->chb_Iterations->isChecked() && !ui->chb_ResEnergy->isChecked())
        criterion = Iterations;
    if(ui->chb_Iterations->isChecked() && ui->chb_ResEnergy->isChecked())
        criterion = Both;
    if(ui->chb_ResEnergy->isChecked() && !ui->chb_Iterations->isChecked())
        criterion = SignalEnergy;

    if(ui->btt_Calc->text()== "calculate")
    {
        ui->progressBarCalc->setValue(0);
        ui->progressBarCalc->setHidden(false);

        if(_signal_matrix.rows() == 0)
        {
            QString title = "Warning";
            QString text = "No signalfile found.";
            QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
            msgBox.exec();
            return;
        }

        if(ui->chb_Iterations->checkState()  == Qt::Unchecked && ui->chb_ResEnergy->checkState() == Qt::Unchecked)
        {
            QString title = "Error";
            QString text = "No truncation criterion choosen.";
            QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
            msgBox.exec();
            return;
        }

        if(((ui->dsb_energy->value() <= 1 && ui->dsb_energy->isEnabled()) && (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled())) || (ui->dsb_energy->value() <= 1 && ui->dsb_energy->isEnabled() && !ui->sb_Iterations->isEnabled()) || (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled() && !ui->dsb_energy->isEnabled()) )
        {
            QSettings settings;
            if(settings.value("show_warnings", true).toBool())
            {
                processdurationmessagebox* msgBox = new processdurationmessagebox(this);
                msgBox->setModal(true);
                msgBox->exec();
                msgBox->close();
            }
        }

        ui->frame->setEnabled(false);
        ui->btt_OpenSignal->setEnabled(false);
        ui->btt_Calc->setText("cancel");
        ui->cb_channels->setEnabled(false);
        ui->cb_all_select->setEnabled(false);
        ui->dsb_from->setEnabled(false);
        ui->dsb_to->setEnabled(false);
        ui->sb_sample_count ->setEnabled(false);

        ui->tbv_Results->setRowCount(0);
        ui->lb_IterationsProgressValue->setText("0");
        ui->lb_RestEnergieResiduumValue->setText("0");
        _adaptive_atom_list.clear();
        _fix_dict_atom_list.clear();
        _is_saved = false;
        _residuum_matrix = _signal_matrix;
        _atom_sum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols());
        callAtomSumWindow->update();
        callResidumWindow->update();

        _counter_time.start();
        _counter_timer->setInterval(100);
        _counter_timer->start();

        if(ui->rb_OwnDictionary->isChecked())
        {
            ui->tbv_Results->setColumnCount(2);
            ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];atom").split(";"));
            ui->tbv_Results->setColumnWidth(0,55);
            ui->tbv_Results->setColumnWidth(1,280);
            _max_tbv_header_width = 0;
            calc_fix_mp(QString(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()), _signal_matrix, criterion);
        }
        else if(ui->rb_adativMp->isChecked())
        {
            ui->tbv_Results->setColumnCount(5);
            ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];scale\n[sec];trans\n[sec];modu\n[Hz];phase\n[rad]").split(";"));
            ui->tbv_Results->setColumnWidth(0,55);
            ui->tbv_Results->setColumnWidth(1,45);
            ui->tbv_Results->setColumnWidth(2,40);
            ui->tbv_Results->setColumnWidth(3,40);
            ui->tbv_Results->setColumnWidth(4,50);

            calc_adaptiv_mp(_signal_matrix, criterion);
        }        
    }
    //cancel calculation thread
    else if(ui->btt_Calc->text() == "cancel")
    {
        mp_Thread->requestInterruption();
        ui->btt_Calc->setText("wait...");
    }
}

//*************************************************************************************************************

void MainWindow::on_time_out()
{
    QTime diff_time(0,0);
    diff_time = diff_time.addMSecs(_counter_time.elapsed());
    ui->lb_timer->setText(diff_time.toString("hh:mm:ss.zzz"));
    _counter_timer->start();
}

//*************************************************************************************************************

qint32 counter = 0;
void MainWindow::recieve_result(qint32 current_iteration, qint32 max_iterations, qreal current_energy, qreal max_energy, MatrixXd residuum,
                                adaptive_atom_list adaptive_atom_res_list, fix_dict_atom_list fix_dict_atom_res_list)
{
    _tbv_is_loading = true;

    qreal percent = ui->dsb_energy->value();
    qreal residuum_energy = 100 * (max_energy - current_energy) / max_energy;

    //remaining energy and iterations update
    ui->lb_IterationsProgressValue->setText(QString::number(current_iteration));
    ui->lb_RestEnergieResiduumValue->setText(QString::number(residuum_energy, 'f', 2) + "%");
    //current atoms list update
    if(fix_dict_atom_res_list.isEmpty())
    {
        GaborAtom temp_atom = adaptive_atom_res_list.last();
        qreal percent_atom_energy = 100 * temp_atom.energy / max_energy;
        qreal phase = temp_atom.phase_list.first();
        if(temp_atom.phase_list.first() > 2*PI)
            phase = temp_atom.phase_list.first() - 2*PI;

        QTableWidgetItem* atomEnergieItem = new QTableWidgetItem(QString::number(percent_atom_energy, 'f', 2));
        QTableWidgetItem* atomScaleItem = new QTableWidgetItem(QString::number(temp_atom.scale / _sample_rate, 'g', 3));
        QTableWidgetItem* atomTranslationItem = new QTableWidgetItem(QString::number(temp_atom.translation / qreal(_sample_rate) + _from, 'g', 4));
        QTableWidgetItem* atomModulationItem = new QTableWidgetItem(QString::number(temp_atom.modulation * _sample_rate / temp_atom.sample_count, 'g', 3));
        QTableWidgetItem* atomPhaseItem = new QTableWidgetItem(QString::number(phase, 'g', 3));

        atomEnergieItem->setFlags(Qt::ItemIsUserCheckable);
        atomScaleItem->setFlags(Qt::NoItemFlags);
        atomTranslationItem->setFlags(Qt::NoItemFlags);
        atomModulationItem->setFlags(Qt::NoItemFlags);
        atomPhaseItem->setFlags(Qt::NoItemFlags);
        atomEnergieItem->setCheckState(Qt::Checked);

        atomEnergieItem->setTextAlignment(0x0082);
        atomScaleItem->setTextAlignment(0x0082);
        atomTranslationItem->setTextAlignment(0x0082);
        atomModulationItem->setTextAlignment(0x0082);
        atomPhaseItem->setTextAlignment(0x0082);

        _adaptive_atom_list.append(temp_atom);
        qSort(_adaptive_atom_list.begin(),_adaptive_atom_list.end(), sort_Energie);
        qSort(adaptive_atom_res_list.begin(),adaptive_atom_res_list.end(), sort_Energie);

        //index = _adaptive_atom_list.indexOf(temp_atom);

        qint32 index = 0;
        while(index < adaptive_atom_res_list.length())
        {
            if(temp_atom.scale == adaptive_atom_res_list.at(index).scale
                    && temp_atom.modulation == adaptive_atom_res_list.at(index).modulation
                    && temp_atom.translation == adaptive_atom_res_list.at(index).translation
                    && temp_atom.energy == adaptive_atom_res_list.at(index).energy)
                break;
            index++;
        }

        ui->tbv_Results->insertRow(index);

        ui->tbv_Results->setItem(index, 0, atomEnergieItem);
        ui->tbv_Results->setItem(index, 1, atomScaleItem);
        ui->tbv_Results->setItem(index, 2, atomTranslationItem);
        ui->tbv_Results->setItem(index, 3, atomModulationItem);
        ui->tbv_Results->setItem(index, 4, atomPhaseItem);


        QList<qint32> sizes = ui->splitter->sizes();
        sizes.insert(0, 240);
        ui->splitter->setSizes(sizes);

        for(qint32 i = 0; i < _signal_matrix.cols(); i++)
        {
            VectorXd atom_vec = temp_atom.max_scalar_list.at(i) * temp_atom.create_real(temp_atom.sample_count, temp_atom.scale, temp_atom.translation, temp_atom.modulation, temp_atom.phase_list.at(i));
            _residuum_matrix.col(i) -= atom_vec;
            _atom_sum_matrix.col(i) += atom_vec;
        }
    }
    else if(adaptive_atom_res_list.isEmpty())
    {
        QFont font;
        QFontMetrics fm(font);
        qint32 header_width = fm.width(fix_dict_atom_res_list.last().display_text) + 35;
        if(header_width > _max_tbv_header_width)
        {
            ui->tbv_Results->setColumnWidth(1, header_width);
            _max_tbv_header_width = header_width;

            QList<qint32> sizes = ui->splitter->sizes();
            sizes.insert(0, header_width + 90);
            ui->splitter->setSizes(sizes);
        }

        FixDictAtom temp_atom = fix_dict_atom_res_list.last();
        ui->tbv_Results->setRowCount(fix_dict_atom_res_list.length());

        qreal percent_atom_energy = 100 * temp_atom.energy / max_energy;

        QTableWidgetItem* atom_energie_item = new QTableWidgetItem(QString::number(percent_atom_energy, 'f', 2));
        QTableWidgetItem* atom_name_item = new QTableWidgetItem(fix_dict_atom_res_list.last().display_text);


        atom_energie_item->setFlags(Qt::ItemIsUserCheckable);
        atom_name_item->setFlags(Qt::NoItemFlags);

        atom_energie_item->setCheckState(Qt::Checked);

        atom_energie_item->setTextAlignment(0x0082);
        atom_name_item->setTextAlignment(0x0081);

         _fix_dict_atom_list.append(temp_atom);

        ui->tbv_Results->setItem(ui->tbv_Results->rowCount()- 1, 0, atom_energie_item);
        ui->tbv_Results->setItem(ui->tbv_Results->rowCount() - 1, 1, atom_name_item);


        for(qint32 i = 0; i < _signal_matrix.cols(); i++)
        {
            _residuum_matrix.col(i) -= fix_dict_atom_res_list.last().max_scalar_list.at(i) * fix_dict_atom_res_list.last().vector_list.first();
            _atom_sum_matrix.col(i) += fix_dict_atom_res_list.last().max_scalar_list.at(i) * fix_dict_atom_res_list.last().vector_list.first();
        }
    }

    //update residuum and atom sum for painting and later save to hdd
    //_residuum_matrix = residuum;
    //_atom_sum_matrix = _signal_matrix - _residuum_matrix;

    //progressbar update
    qint32 prgrsbar_adapt = 99;

    if(max_iterations > 1999 && current_iteration < 100)
             ui->progressBarCalc->setMaximum(100);
    if(ui->chb_ResEnergy->isChecked() && (current_iteration >= (prgrsbar_adapt)) && (max_energy - current_energy) > (0.01 * percent * max_energy))
        ui->progressBarCalc->setMaximum(current_iteration + 5);
    if(max_iterations < 1999)
        ui->progressBarCalc->setMaximum(max_iterations);

    ui->progressBarCalc->setValue(current_iteration);

    if(((current_iteration == max_iterations) || (max_energy - current_energy) < (0.01 * percent * max_energy))&&ui->chb_ResEnergy->isChecked())
        ui->progressBarCalc->setValue(ui->progressBarCalc->maximum());

    if(counter % 10 == 0)
    {
    callAtomSumWindow->update();
    callResidumWindow->update();
    }
    _tbv_is_loading = false;
    counter++;

}


//*************************************************************************************************************

void MainWindow::recieve_warnings(qint32 warning_number)
{

}

//*************************************************************************************************************

void MainWindow::tbv_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    Q_UNUSED(bottomRight);
    bool all_selected = true;
    bool all_deselected = true;

    if(_tbv_is_loading) return;

    for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
        if(ui->tbv_Results->item(i, 0)->checkState()) all_deselected = false;
        else all_selected = false;

    if(all_selected) ui->cb_all_select->setCheckState(Qt::Checked);
    else if(all_deselected) ui->cb_all_select->setCheckState(Qt::Unchecked);
    else ui->cb_all_select->setCheckState(Qt::PartiallyChecked);

    QTableWidgetItem* item = ui->tbv_Results->item(topLeft.row(), 0);
    if(topLeft.row() == ui->tbv_Results->rowCount() - 1)
    {
        if(item->checkState())
        {
            for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)            
                _atom_sum_matrix.col(channels) += _real_residuum_matrix.col(channels);            
        }
        else
        {
            for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)            
                _atom_sum_matrix.col(channels) -= _real_residuum_matrix.col(channels);            
        }
    }
    else
    {
        if(ui->tbv_Results->columnCount() > 2)
        {
            GaborAtom  atom = _adaptive_atom_list.at(topLeft.row());
            if(!_auto_change)
                _select_atoms_map[topLeft.row()] = item->checkState();

            if(item->checkState())
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                    _residuum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                }
            }
            else
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                    _residuum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                }
            }
        }
        else
        {
            FixDictAtom  atom = _fix_dict_atom_list.at(topLeft.row());
            if(!_auto_change)
                _select_atoms_map[topLeft.row()] = item->checkState();

            if(item->checkState())
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.vector_list.first();
                    _residuum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.vector_list.first();
                }
            }
            else
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.vector_list.first();
                    _residuum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.vector_list.first();
                }
            }
        }
    }
    callAtomSumWindow->update();
    callResidumWindow->update();
}

//*************************************************************************************************************

void MainWindow::calc_thread_finished()
{
    _tbv_is_loading = true;

    _counter_timer->stop();
    ui->frame->setEnabled(true);
    ui->btt_OpenSignal->setEnabled(true);

    ui->btt_Calc->setText("calculate");
    ui->cb_channels->setEnabled(true);
    ui->cb_all_select->setEnabled(true);
    ui->dsb_from->setEnabled(true);
    ui->dsb_to->setEnabled(true);
    ui->sb_sample_count ->setEnabled(true);

    for(qint32 col = 0; col < ui->tbv_Results->columnCount(); col++)
        for(qint32 row = 0; row < ui->tbv_Results->rowCount(); row++)
        {
            if(col == 0)
                ui->tbv_Results->item(row, col)->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            else
                ui->tbv_Results->item(row, col)->setFlags(Qt::ItemIsEnabled);
        }

    _real_residuum_matrix = _residuum_matrix;

    for(qint32 i = 0; i < ui->tbv_Results->rowCount(); i++)
        _select_atoms_map.insert(i, true);

    ui->tbv_Results->setRowCount(ui->tbv_Results->rowCount() + 1);

    QTableWidgetItem* energy_item = new QTableWidgetItem(ui->lb_RestEnergieResiduumValue->text().remove('%'));
    energy_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    energy_item->setCheckState(Qt::Unchecked);
    energy_item->setTextAlignment(0x0082);

    QTableWidgetItem* residuum_item = new QTableWidgetItem("residuum");
    residuum_item->setFlags(Qt::ItemIsEnabled);
    residuum_item->setTextAlignment(Qt::AlignCenter);

    ui->tbv_Results->setItem(ui->tbv_Results->rowCount() - 1, 0, energy_item);
    ui->tbv_Results->setItem(ui->tbv_Results->rowCount() - 1, 1, residuum_item);
    ui->tbv_Results->setSpan(ui->tbv_Results->rowCount() - 1, 1, 1, 4);

    _tbv_is_loading = false;

    update();
}

//*************************************************************************************************************

void MainWindow::calc_adaptiv_mp(MatrixXd signal, TruncationCriterion criterion)
{
    adaptive_Mp = new AdaptiveMp();
    qreal res_energy = ui->dsb_energy->value();

    //threading
    mp_Thread = new QThread;
    adaptive_Mp->moveToThread(mp_Thread);

    connect(this, SIGNAL(send_input(MatrixXd, qint32, qreal, bool, qint32, qint32, qreal, qreal, qreal, qreal)),
            adaptive_Mp, SLOT(recieve_input(MatrixXd, qint32, qreal, bool, qint32, qint32, qreal, qreal, qreal, qreal)));
    connect(adaptive_Mp, SIGNAL(current_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)),
                 this, SLOT(recieve_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)));
    connect(adaptive_Mp, SIGNAL(finished_calc()), mp_Thread, SLOT(quit()));
    connect(adaptive_Mp, SIGNAL(finished_calc()), adaptive_Mp, SLOT(deleteLater()));
    connect(mp_Thread, SIGNAL(finished()), this, SLOT(calc_thread_finished()));
    connect(mp_Thread, SIGNAL(finished()), mp_Thread, SLOT(deleteLater()));

    QSettings settings;
    bool fixphase = settings.value("fixPhase", false).toBool();
    qint32 boost = settings.value("boost", 100).toInt();
    qint32 iterations = settings.value("adaptive_iterations", 1E3).toInt();
    qreal reflection = settings.value("adaptive_reflection", 1.00).toDouble();
    qreal expansion = settings.value("adaptive_expansion", 0.20).toDouble();
    qreal contraction = settings.value("adaptive_contraction", 0.5).toDouble();
    qreal fullcontraction = settings.value("adaptive_fullcontraction", 0.50).toDouble();
    switch(criterion)
    {
        case Iterations:        
            emit send_input(signal, ui->sb_Iterations->value(), qreal(MININT32), fixphase, boost, iterations,
                            reflection, expansion, contraction, fullcontraction);
            mp_Thread->start();        
            break;

        case SignalEnergy:        
            emit send_input(signal, MAXINT32, res_energy, fixphase, boost, iterations,
                            reflection, expansion, contraction, fullcontraction);
            mp_Thread->start();        
            break;

        case Both:
            emit send_input(signal, ui->sb_Iterations->value(), res_energy, fixphase, boost, iterations,
                            reflection, expansion, contraction, fullcontraction);
            mp_Thread->start();        
            break;
    }       
}

//************************************************************************************************************************************

void MainWindow::calc_fix_mp(QString path, MatrixXd signal, TruncationCriterion criterion)
{
    fixDict_Mp = new FixDictMp();
    qreal res_energy = ui->dsb_energy->value();

    //threading
    mp_Thread = new QThread;
    fixDict_Mp->moveToThread(mp_Thread);    

    connect(this, SIGNAL(send_input_fix_dict(MatrixXd, qint32, qreal, qint32, QString)),
            fixDict_Mp, SLOT(recieve_input(MatrixXd, qint32, qreal, qint32, QString)));
    connect(fixDict_Mp, SIGNAL(current_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)),
                  this, SLOT(recieve_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)));
    connect(fixDict_Mp, SIGNAL(finished_calc()), mp_Thread, SLOT(quit()));
    connect(fixDict_Mp, SIGNAL(finished_calc()), fixDict_Mp, SLOT(deleteLater()));
    connect(mp_Thread, SIGNAL(finished()), this, SLOT(calc_thread_finished()));
    connect(mp_Thread, SIGNAL(finished()), mp_Thread, SLOT(deleteLater()));

    connect(fixDict_Mp, SIGNAL(send_warning(qint32)), this, SLOT(recieve_warnings(qint32)));

    QSettings settings;
    qint32 boost = settings.value("boost", 100).toInt();

    switch(criterion)
    {
        case Iterations:
            emit send_input_fix_dict(signal, ui->sb_Iterations->value(), qreal(MININT32), boost, path);
            mp_Thread->start();
            break;

        case SignalEnergy:
            emit send_input_fix_dict(signal, MAXINT32, res_energy, boost, path);
            mp_Thread->start();
            break;

        case Both:
            emit send_input_fix_dict(signal, ui->sb_Iterations->value(), res_energy, boost, path);
            mp_Thread->start();
            break;
    }
}

//*****************************************************************************************************************

// Opens Dictionaryeditor
void MainWindow::on_actionW_rterbucheditor_triggered()
{        
    if(_editor_window == NULL)
    {
        _editor_window = new EditorWindow();
        connect(_editor_window, SIGNAL(dict_saved()), this, SLOT(on_dicts_saved()));
    }
    if(!_editor_window->isVisible()) _editor_window->show();
    else
    {
        _editor_window->setWindowState(Qt::WindowActive);
        _editor_window->raise();
    }

}

//*****************************************************************************************************************

// opens advanced Dictionaryeditor
void MainWindow::on_actionErweiterter_W_rterbucheditor_triggered()
{
     if(_enhanced_editor_window == NULL)
     {
         _enhanced_editor_window = new Enhancededitorwindow();
         if(_editor_window == NULL) _editor_window = new EditorWindow();
         connect(_enhanced_editor_window, SIGNAL(dict_saved()), _editor_window, SLOT(on_save_dicts()));
     }
     if(!_enhanced_editor_window->isVisible()) _enhanced_editor_window->show();
     else
     {
         _enhanced_editor_window->setWindowState(Qt::WindowActive);
         _enhanced_editor_window->raise();
     }
}

//*****************************************************************************************************************


// opens formula editor
void MainWindow::on_actionAtomformeleditor_triggered()
{
    if(_formula_editor == NULL)
    {
        _formula_editor = new Formulaeditor();
        if(_enhanced_editor_window == NULL) _enhanced_editor_window = new Enhancededitorwindow();
        connect(_formula_editor, SIGNAL(formula_saved()), _enhanced_editor_window, SLOT(on_formula_saved()));
    }
    if(!_formula_editor->isVisible()) _formula_editor->show();
    else
    {
        _formula_editor->setWindowState(Qt::WindowActive);
        _formula_editor->raise();
    }
}

//*****************************************************************************************************************

// open treebase window
void MainWindow::on_actionCreate_treebased_dictionary_triggered()
{
    TreebasedDictWindow *x = new TreebasedDictWindow();
    x->show();
}

//*****************************************************************************************************************

// open settings
void MainWindow::on_actionSettings_triggered()
{
    if(_setting_window == NULL) _setting_window = new settingwindow();
    _setting_window->set_values();
    if(!_setting_window->isVisible()) _setting_window->show();
    else
    {
        _setting_window->setWindowState(Qt::WindowActive);
        _setting_window->raise();
    }
}

//*****************************************************************************************************************

// opens Filedialog for read signal (contextmenue)
void MainWindow::on_actionNeu_triggered()
{
    open_file();
}

//*****************************************************************************************************************

// opens Filedialog for read signal (button)
void MainWindow::on_btt_OpenSignal_clicked()
{
    open_file();
}


//*****************************************************************************************************************

void MainWindow::on_dsb_sample_rate_editingFinished()
{
    _sample_rate = ui->dsb_sample_rate->value();
}

//*****************************************************************************************************************

void MainWindow::on_dsb_from_editingFinished()
{   
    _from = ui->dsb_from->value();
    read_fiff_file_new(_file_name);
}

//*****************************************************************************************************************

void MainWindow::on_dsb_to_editingFinished()
{   
    _to = ui->dsb_to->value();
    read_fiff_file_new(_file_name);
}

//*****************************************************************************************************************

void MainWindow::on_dsb_from_valueChanged(double arg1)
{
    _come_from_from = true;
    qreal var = (_to - arg1) * _sample_rate;
    if(ui->dsb_to->value() <= arg1 || var < 64 || var > 4097)
        ui->dsb_from->setValue(_from);
    else
        ui->sb_sample_count->setValue(var);
    _come_from_from = false;
}

//*****************************************************************************************************************

void MainWindow::on_dsb_to_valueChanged(double arg1)
{
    qreal var  = (arg1 - _from) * _sample_rate;
    if(ui->dsb_from->value() >= arg1 || var < 64 || var > 4097)
        ui->dsb_to->setValue(_to);

    if(!_come_from_sample_count)
        ui->sb_sample_count->setValue(var);
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_count_valueChanged(int arg1)
{
    _come_from_sample_count = true;
    if(!_come_from_from)
        ui->dsb_to->setValue( _from  + ((qreal)arg1 / _sample_rate));
    _come_from_sample_count = false;
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_count_editingFinished()
{
    if(!_come_from_from)
    {
        _to = ui->dsb_to->value();
        read_fiff_file_new(_file_name);
    }
}

//*****************************************************************************************************************

void MainWindow::on_cb_all_select_clicked()
{
    if(_tbv_is_loading) return;

    if( ui->cb_all_select->checkState() == Qt::Unchecked && !_was_partialchecked)
    {
        ui->cb_all_select->setCheckState(Qt::PartiallyChecked);
        _was_partialchecked = true;
    }
    else if(ui->cb_all_select->checkState() == Qt::Checked && !_was_partialchecked)
    {
        ui->cb_all_select->setCheckState(Qt::Unchecked);
        _was_partialchecked = false;
    }

    _auto_change = true;

    if(ui->cb_all_select->checkState() == Qt::Checked)
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            ui->tbv_Results->item(i, 0)->setCheckState(Qt::Checked);
    else if(ui->cb_all_select->checkState() == Qt::Unchecked)
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            ui->tbv_Results->item(i, 0)->setCheckState(Qt::Unchecked);
    else
    {
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            if(_select_atoms_map[i] == true)
                ui->tbv_Results->item(i, 0)->setCheckState(Qt::Checked);
            else
                ui->tbv_Results->item(i, 0)->setCheckState(Qt::Unchecked);
    }


    bool all_selected = true;
    bool all_deselected = true;
    for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)         // last item is residuum
        if(ui->tbv_Results->item(i, 0)->checkState())
            all_deselected = false;
        else
            all_selected = false;

    if(all_selected)
        ui->cb_all_select->setCheckState(Qt::Checked);
    else if(all_deselected)
    {
        ui->cb_all_select->setCheckState(Qt::Unchecked);
        _was_partialchecked = true;
    }
    else ui->cb_all_select->setCheckState(Qt::PartiallyChecked);

    _auto_change = false;
}

//*****************************************************************************************************************

void MainWindow::on_dicts_saved()
{
    fill_dict_combobox();
}

//*****************************************************************************************************************

void MainWindow::on_actionSpeicher_triggered()
{
    if(_file_name.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No file for save."));
        return;
    }

    if(ui->tbv_Results->rowCount() == 0)
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No results for save."));
        return;
    }

    if(_save_path.isEmpty())
    {        
        QString save_name = "";
        QStringList saveList = _file_name.split('/').last().split('.').first().split('_');
        for(int i = 0; i < saveList.length(); i++)
        {
            if(i == saveList.length() - 1)
                save_name += "mp_" + saveList.at(i);
            else
                save_name += saveList.at(i) + "_";
        }
        _save_path = QFileDialog::getSaveFileName(this, "Save file as...", _last_save_path + "/" + save_name,"(*.fif)");
        if(_save_path.isEmpty()) return;
    }
    QStringList string_list = _save_path.split('/');
    _last_save_path = "";
    for(qint32 i = 0; i < string_list.length() - 1; i++)
        _last_save_path += string_list.at(i) + '/';
    save_fif_file();

    save_fif_file();
}

//*****************************************************************************************************************

void MainWindow::on_actionSpeicher_unter_triggered()
{
    if(_file_name.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No file for save."));
        return;
    }

    if(_file_name.split('.').last() != "fif")
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No fif file for save."));
        return;
    }

    if(ui->tbv_Results->rowCount() == 0)
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No results for save."));
        return;
    }

    QString save_name = "";
    QStringList saveList = _file_name.split('/').last().split('.').first().split('_');
    for(int i = 0; i < saveList.length(); i++)
    {
        if(i == saveList.length() - 1)
            save_name += "mp_" + saveList.at(i);
        else
            save_name += saveList.at(i) + "_";
    }

    _save_path = QFileDialog::getSaveFileName(this, "Save file as...", _last_save_path + "/" + save_name,"(*.fif)");
    if(_save_path.isEmpty()) return;
    else
    {
        QStringList string_list = _save_path.split('/');
        _last_save_path = "";
        for(qint32 i = 0; i < string_list.length() - 1; i++)
            _last_save_path += string_list.at(i) + '/';
        save_fif_file();
    }
}

//*****************************************************************************************************************

void MainWindow::save_fif_file()
{
    //change ui
    ui->lb_timer->setHidden(true);
    ui->cb_all_select->setHidden(true);
    ui->lb_save_file->setHidden(false);


    QFile t_fileIn(_file_name);
    QFile t_fileOut(_save_path);

    //
    //   Setup for reading the raw data
    //
    FiffRawData raw(t_fileIn);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    QStringList include;
    include << "STI 014";
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;
    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);

    MatrixXd cals;
    FiffStream::SPtr outfid = Fiff::start_writing_raw(t_fileOut,raw.info, cals, picks);

    //
    //   Set up the reading parameters
    //
    fiff_int_t from = raw.first_samp;
    fiff_int_t to = raw.last_samp;
    float quantum_sec = 10.0f;//read and write in 10 sec junks
    fiff_int_t quantum = ceil(quantum_sec*raw.info.sfreq);  //   To read the whole file at once set quantum     = to - from + 1;

    //************************************************************************************

    //
    //   Read and write all the data
    //
    bool first_buffer = true;
    fiff_int_t first, last;
    MatrixXd data;
    MatrixXd times;
    qreal start_change = _from * raw.info.sfreq;    // start of change
    qreal end_change = _to * raw.info.sfreq + 1;    // end of change

    ui->progressBarCalc->setValue(0);
    ui->progressBarCalc->setMinimum(0);
    ui->progressBarCalc->setMaximum(to);

    // from 0 to start of change
    for(first = from; first < start_change; first+=quantum)
    {
        last = first+quantum-1;
        if (last > start_change)
        {
            last = start_change;
        }
        if (!raw.read_raw_segment(data ,times, first, last, picks))
        {
                printf("error during read_raw_segment\n");
                QMessageBox::warning(this, tr("Error"),
                tr("error: Save unsucessful."));
                return;
        }
        printf("Writing...");
        if (first_buffer)
        {
           if (first > 0)
               outfid->write_int(FIFF_FIRST_SAMPLE,&first);
           first_buffer = false;
        }
        outfid->write_raw_buffer(data,cals);
        printf("[done]\n");

        ui->progressBarCalc->setValue(first);
    }

    //************************************************************************************

    // from start of change to end of change
    if (!raw.read_raw_segment(data, times, start_change ,end_change,picks))
    {
            printf("error during read_raw_segment\n");
            QMessageBox::warning(this, tr("Error"),
            tr("error: Save unsucessful."));
            return;
    }

    qint32 index = 0;
    for(qint32 channels = 0; channels < data.rows(); channels++)
    {
        if(_select_channel_map[channels])
        {
            data.row(channels) =  _atom_sum_matrix.col(index)  ;
            index++;
        }
    }

    printf("Writing new data...");
    outfid->write_raw_buffer(data,cals);
    printf("[done]\n");

    //************************************************************************************

    // from end of change to end
    for(first = end_change; first < to; first+=quantum)
    {
        last = first+quantum-1;
        if (last > to)
        {
            last = to;
        }
        if (!raw.read_raw_segment(data,times,first,last, picks))
        {
                printf("error during read_raw_segment\n");
                QMessageBox::warning(this, tr("Error"),
                tr("error: Save unsucessful."));
                return;
        }
        printf("Writing...");
        outfid->write_raw_buffer(data,cals);
        printf("[done]\n");
        ui->progressBarCalc->setValue(first);
    }

    save_parameters();
    ui->progressBarCalc->setValue(to);

    printf("Writing...");
    outfid->write_raw_buffer(data,cals);
    printf("[done]\n");

    outfid->finish_writing_raw();
    printf("Finished\n");

    _is_saved = true;
    ui->lb_timer->setHidden(false);
    ui->cb_all_select->setHidden(false);
    ui->lb_save_file->setHidden(true);
}

//*****************************************************************************************************************

void MainWindow::save_parameters()
{
    QString save_parameter_path = _save_path.split(".").first() + ".txt";
    QFile xml_file(save_parameter_path);
    if(xml_file.open(QIODevice::WriteOnly))
    {
        QXmlStreamWriter xmlWriter(&xml_file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        for(qint32 i = 0; i < _adaptive_atom_list.length(); i++)
        {
            if(ui->tbv_Results->columnCount() == 2 && ui->tbv_Results->item(i, 1)->text() != "residuum")
            {
                FixDictAtom fix_atom = _fix_dict_atom_list.at(i);

                xmlWriter.writeStartElement("ATOM");
                xmlWriter.writeAttribute("formula", fix_atom.atom_formula);
                xmlWriter.writeAttribute("sample_count", QString::number(fix_atom.sample_count));
                xmlWriter.writeAttribute("energy", ui->tbv_Results->item(i, 0)->text());
                xmlWriter.writeAttribute("parameters", ui->tbv_Results->item(i, 1)->text());
                xmlWriter.writeAttribute("dict_source", fix_atom.dict_source);

                xmlWriter.writeStartElement("PARAMETER");
                if(fix_atom.type == AtomType::GABORATOM)
                {
                    xmlWriter.writeAttribute("formula", "GABORATOM");
                    xmlWriter.writeStartElement("PARAMETER");
                    xmlWriter.writeAttribute("scale", QString::number(fix_atom.gabor_atom.scale));
                    xmlWriter.writeAttribute("translation", QString::number(fix_atom.translation));
                    xmlWriter.writeAttribute("modulation", QString::number(fix_atom.gabor_atom.modulation));
                    xmlWriter.writeAttribute("phase", QString::number(fix_atom.gabor_atom.phase));
                }
                else if(fix_atom.type == AtomType::CHIRPATOM)
                {
                    xmlWriter.writeAttribute("formula", "CHIRPATOM");
                    xmlWriter.writeStartElement("PARAMETER");
                    xmlWriter.writeAttribute("scale", QString::number(fix_atom.chirp_atom.scale));
                    xmlWriter.writeAttribute("translation", QString::number(fix_atom.translation));
                    xmlWriter.writeAttribute("modulation", QString::number(fix_atom.chirp_atom.modulation));
                    xmlWriter.writeAttribute("phase", QString::number(fix_atom.chirp_atom.phase));
                    xmlWriter.writeAttribute("chirp", QString::number(fix_atom.chirp_atom.chirp));
                }
                else if(fix_atom.type == AtomType::FORMULAATOM)
                {
                    xmlWriter.writeAttribute("translation", QString::number(fix_atom.translation));
                    xmlWriter.writeAttribute("a", QString::number(fix_atom.formula_atom.a));
                    xmlWriter.writeAttribute("b", QString::number(fix_atom.formula_atom.b));
                    xmlWriter.writeAttribute("c", QString::number(fix_atom.formula_atom.c));
                    xmlWriter.writeAttribute("d", QString::number(fix_atom.formula_atom.d));
                    xmlWriter.writeAttribute("e", QString::number(fix_atom.formula_atom.e));
                    xmlWriter.writeAttribute("f", QString::number(fix_atom.formula_atom.f));
                    xmlWriter.writeAttribute("g", QString::number(fix_atom.formula_atom.g));
                    xmlWriter.writeAttribute("h", QString::number(fix_atom.formula_atom.h));
                }
                xmlWriter.writeEndElement();    //PARAMETER
                xmlWriter.writeEndElement();    //ATOM
            }
            else
            {
                GaborAtom gabor_atom = _adaptive_atom_list.at(i);
                xmlWriter.writeStartElement("ATOM");
                xmlWriter.writeAttribute("formula", "GABORATOM");
                xmlWriter.writeAttribute("sample_count", QString::number(gabor_atom.sample_count));
                xmlWriter.writeAttribute("energy", ui->tbv_Results->item(i, 0)->text());

                xmlWriter.writeStartElement("PARAMETER");
                xmlWriter.writeAttribute("scale", QString::number(gabor_atom.scale));
                xmlWriter.writeAttribute("translation", QString::number(gabor_atom.translation));
                xmlWriter.writeAttribute("modulation", QString::number(gabor_atom.modulation));
                xmlWriter.writeAttribute("phase", QString::number(gabor_atom.phase));

                xmlWriter.writeEndElement();    //PARAMETER
                xmlWriter.writeEndElement();    //ATOM
            }
        }
        xmlWriter.writeEndDocument();
    }
    xml_file.close();
}

//*****************************************************************************************************************

void MainWindow::on_actionExport_triggered()
{
    if(_adaptive_atom_list.length() == 0)
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No adaptive MP results for save."));
        return;
    }


    QString save_path = QFileDialog::getSaveFileName(this, "Export results as dict file...", QDir::homePath() + "/" + "Matching-Pursuit-Toolbox" + "/" + "resultdict","(*.dict)");
    if(save_path.isEmpty()) return;

    QStringList string_list = save_path.split('/');
    _last_save_path = "";
    for(qint32 i = 0; i < string_list.length() - 1; i++)
        _last_save_path += string_list.at(i) + '/';

    QFile xml_file(save_path);
    if(xml_file.open(QIODevice::WriteOnly))
    {
        QXmlStreamWriter xmlWriter(&xml_file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        xmlWriter.writeStartElement("COUNT");
        xmlWriter.writeAttribute("of_atoms", QString::number(_adaptive_atom_list.length()));
        xmlWriter.writeStartElement("built_Atoms");
        xmlWriter.writeAttribute("formula", "Gaboratom");
        xmlWriter.writeAttribute("sample_count", QString::number(_adaptive_atom_list.first().sample_count));
        xmlWriter.writeAttribute("atom_count", QString::number(_adaptive_atom_list.length()));
        xmlWriter.writeAttribute("source_dict", save_path.split('/').last().split('.').first());

        for(qint32 i = 0; i < _adaptive_atom_list.length(); i++)
        {
            GaborAtom gabor_atom = _adaptive_atom_list.at(i);
            QStringList result_list = gabor_atom.create_string_values(gabor_atom.sample_count, gabor_atom.scale, gabor_atom.sample_count / 2, gabor_atom.modulation, gabor_atom.phase);

            xmlWriter.writeStartElement("ATOM");
            xmlWriter.writeAttribute("ID", QString::number(i));
            xmlWriter.writeAttribute("scale", QString::number(gabor_atom.scale));
            xmlWriter.writeAttribute("modu", QString::number(gabor_atom.modulation));
            xmlWriter.writeAttribute("phase", QString::number(gabor_atom.phase));

            xmlWriter.writeStartElement("samples");
            QString samples_to_xml;
            for (qint32 it = 0; it < result_list.length(); it++)
            {
                samples_to_xml.append(result_list.at(it));
                samples_to_xml.append(":");
            }
            xmlWriter.writeAttribute("samples", samples_to_xml);
            xmlWriter.writeEndElement();    //samples

            xmlWriter.writeEndElement();    //ATOM

        }
        xmlWriter.writeEndElement();    //bulit_atoms
        xmlWriter.writeEndElement();    //COUNT
        xmlWriter.writeEndDocument();
    }
    xml_file.close();
    fill_dict_combobox();
}

//*****************************************************************************************************************

bool MainWindow::sort_Energie(const GaborAtom atom_1, const GaborAtom atom_2)
{
    return (atom_1.energy > atom_2.energy);
}

//*****************************************************************************************************************

