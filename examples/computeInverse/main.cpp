//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Example of computing a L2 minimum-norm estimate or a dSPM solution
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff_evoked_set.h>
#include <mne/mne_inverse_operator.h>

#include <fiff/fiff_evoked.h>
#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
* The function main marks the entry point of the program.
* By default, main has the storage class extern.
*
* @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
* @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
* @return the value that was set to exit() (which is 0 if exit() is called via quit()).
*/
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Compute Inverse Example");
    parser.addHelpOption();
    QCommandLineOption sampleEvokedFileOption("e", "Path to evoked <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption sampleInvFileOption("i", "Path to inverse operator <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif");
    QCommandLineOption snrOption("s", "<snr> of the given evoked file.", "snr", "1.0");
    QCommandLineOption methodOption("m", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");
    QCommandLineOption stcFileOption("t", "Path to <target> where stc is stored to.", "target", "");
    parser.addOption(sampleEvokedFileOption);
    parser.addOption(sampleInvFileOption);
    parser.addOption(snrOption);
    parser.addOption(methodOption);
    parser.addOption(stcFileOption);
    parser.process(app);

//  setno       - Data set number
//  nave        - Number of averages (scales the noise covariance)
//             If negative, the number of averages in the data will be
//             used
//  lambda2     - The regularization factor
//  dSPM        - do dSPM?
//  sLORETA     - do sLORETA?

    QFile t_fileEvoked(parser.value(sampleEvokedFileOption));
    QFile t_fileInv(parser.value(sampleInvFileOption));

    float snr = parser.value(snrOption).toFloat();
    QString method(parser.value(methodOption)); //"MNE" | "dSPM" | "sLORETA"
    QString t_sFileNameStc(parser.value(stcFileOption));

    // Parse command line parameters
    for(qint32 i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "-snr") == 0 || strcmp(argv[i], "--snr") == 0)
        {
            if(i + 1 < argc)
                snr = atof(argv[i+1]);
        }
        else if(strcmp(argv[i], "-method") == 0 || strcmp(argv[i], "--method") == 0)
        {
            if(i + 1 < argc)
                method = QString::fromUtf8(argv[i+1]);
        }
        else if(strcmp(argv[i], "-stc") == 0 || strcmp(argv[i], "--stc") == 0)
        {
            if(i + 1 < argc)
                t_sFileNameStc = QString::fromUtf8(argv[i+1]);
        }
    }

    double lambda2 = 1.0 / pow(snr, 2);
    qDebug() << "Start calculation with: SNR" << snr << "; Lambda" << lambda2 << "; Method" << method << "; stc:" << t_sFileNameStc;

    //
    //   Read the data first
    //
    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return 1;

    //
    //   Then the inverse operator
    //
    MNEInverseOperator inverse_operator(t_fileInv);

    //
    // Compute inverse solution
    //
    MinimumNorm minimumNorm(inverse_operator, lambda2, method);
    MNESourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);

    //
    //Results
    //
    std::cout << "\npart ( block( 0, 0, 10, 10) ) of the inverse solution:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;
    printf("tmin = %f s\n", sourceEstimate.tmin);
    printf("tstep = %f s\n", sourceEstimate.tstep);


    if(!t_sFileNameStc.isEmpty())
    {
        QFile t_fileStc(t_sFileNameStc);
        sourceEstimate.write(t_fileStc);

        //test if everything was written correctly
        MNESourceEstimate readSourceEstimate(t_fileStc);

        std::cout << "\npart ( block( 0, 0, 10, 10) ) of the inverse solution:\n" << readSourceEstimate.data.block(0,0,10,10) << std::endl;
        printf("tmin = %f s\n", readSourceEstimate.tmin);
        printf("tstep = %f s\n", readSourceEstimate.tstep);
    }

    return 0;//app.exec();
}
