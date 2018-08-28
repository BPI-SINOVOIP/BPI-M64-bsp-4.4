/************************************************************************
* Copyright (C) 2010-2017, Focaltech Systems (R)��All Rights Reserved.
*
* File Name: Focaltech_test_ft3D47.c
*
* Author: Software Development Team, AE
*
* Created: 2016-11-16
*
* Abstract: test item for FT3D47
*
************************************************************************/

/*******************************************************************************
* Included header files
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "../include/focaltech_test_detail_threshold.h"
#include "../include/focaltech_test_supported_ic.h"
#include "../include/focaltech_test_main.h"
#include "../focaltech_test_config.h"

#if (FTS_CHIP_TEST_TYPE ==FT3D47_TEST)

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/

/////////////////////////////////////////////////Reg
#define     DEVIDE_MODE_ADDR            0x00
#define     REG_LINE_NUM                0x01
#define     REG_TX_NUM                  0x02
#define     REG_RX_NUM                  0x03
#define     REG_PATTERN_3C47            0x53

#define     REG_ScCbBuf0                0x4E
#define     REG_ScWorkMode              0x44
#define     REG_ScCbAddrR               0x45
#define     REG_RawBuf0                 0x36
#define     REG_WATER_CHANNEL_SELECT    0x09

#define     REG_TX_NOMAPPING_NUM        0x55
#define     REG_RX_NOMAPPING_NUM        0x56
#define     REG_PATTERN_5422            0X53
#define     REG_MAPPING_SWITCH          0X54
#define     REG_VA_MUL                  0X22
#define     REG_KEY_NUL                 0X21
#define     REG_SHIFT                   0X23
#define     REG_TX_CLK_NUM              0X33
#define     REG_SAMPLE_MOD              0X35
#define     REG_SCAN_VOL                0X20
#define     REG_NORMALIZE_TYPE          0X16
#define     REG_FREQUENCY               0x0A
#define     REG_FIR                     0XFB
#define     REG_RELEASECODEID_H         0xAE
#define     REG_RELEASECODEID_L         0xAF
#define     REG_FW_PROCESS              0x1A
#define     REG_REAL_TX_NUM             0XEB
#define     REG_REAL_RX_NUM             0XEC

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/
enum WaterproofType
{
    WT_NeedProofOnTest,
    WT_NeedProofOffTest,
    WT_NeedTxOnVal,
    WT_NeedRxOnVal,
    WT_NeedTxOffVal,
    WT_NeedRxOffVal,
};
/*******************************************************************************
* Static variables
*******************************************************************************/

static int m_RawData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};

/* Force RawData */
static int m_Force_RawData_Water[2][TX_NUM_MAX] = {{0,0}};
static int m_Force_CBData_Water[2][TX_NUM_MAX] = {{0,0}};
static int m_Force_RawData_Normal[2][TX_NUM_MAX] = {{0,0}};
static int m_Force_CBData_Normal[2][TX_NUM_MAX] = {{0,0}};


static int m_iTempRawData[TX_NUM_MAX * RX_NUM_MAX] = {0};
static unsigned char m_ucTempData[TX_NUM_MAX * RX_NUM_MAX*2] = {0};
static bool m_bV3TP = false;

static unsigned char m_iAllTx=0;
static unsigned char m_iAllRx=0;
static int m_iForceTouchTx=0;
static int m_iForceTouchRx=0;

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
extern struct stCfg_FT3D47_BasicThreshold g_stCfg_FT3D47_BasicThreshold;

/*******************************************************************************
* Static function prototypes
*******************************************************************************/
//////////////////////////////////////////////Communication function
static int StartScan(void);
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer);
static unsigned char GetPanelRows(unsigned char *pPanelRows);
static unsigned char GetPanelCols(unsigned char *pPanelCols);
static unsigned char GetTxSC_CB(unsigned char index, unsigned char *pcbValue);
//////////////////////////////////////////////Common function
static unsigned char GetRawData(void);
static unsigned char GetChannelNum(void);
//////////////////////////////////////////////about Test
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount);
static void ShowRawData(void);
static boolean GetTestCondition(int iTestType, unsigned char ucChannelValue);
boolean GetWaterproofMode(int iTestType, unsigned char ucChannelValue);


boolean FT3D47_StartTest(void);
int FT3D47_get_test_data(char *pTestData);//pTestData, External application for memory, buff size >= 1024*80

unsigned char FT3D47_TestItem_EnterFactoryMode(void);
unsigned char FT3D47_TestItem_RawDataTest(bool * bTestResult);
unsigned char FT3D47_TestItem_SCapRawDataTest(bool* bTestResult);
unsigned char FT3D47_TestItem_SCapCbTest(bool* bTestResult);
unsigned char FT3D47_TestItem_ForceTouch_SCapRawDataTest(bool* bTestResult);
unsigned char FT3D47_TestItem_ForceTouch_SCapCbTest(bool* bTestResult);

/************************************************************************
* Name: FT3D47_StartTest
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/
boolean FT3D47_StartTest()
{

    bool bTestResult = true;
    bool bTempResult = 1;
    unsigned char ReCode=0;
    unsigned char ucDevice = 0;
    int iItemCount=0;

    FTS_TEST_DBG("FT3D47_StartTest");

    //--------------1. Init part
    if (InitTest() < 0)
    {
        FTS_TEST_ERROR("[focal] Failed to init test.");
        return false;
    }

    //--------------2. test item
    if (0 == g_TestItemNum)
        bTestResult = false;

    for (iItemCount = 0; iItemCount < g_TestItemNum; iItemCount++)
    {
        m_ucTestItemCode = g_stTestItem[ucDevice][iItemCount].ItemCode;

        ///////////////////////////////////////////////////////FT3D47_ENTER_FACTORY_MODE
        if (Code_FT3D47_ENTER_FACTORY_MODE == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT3D47_TestItem_EnterFactoryMode();
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }



        ///////////////////////////////////////////////////////FT3D47_RAWDATA_TEST
        if (Code_FT3D47_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT3D47_TestItem_RawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }


        ///////////////////////////////////////////////////////FT3D47_SCAP_CB_TEST
        if (Code_FT3D47_SCAP_CB_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT3D47_TestItem_SCapCbTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT3D47_SCAP_RAWDATA_TEST
        if (Code_FT3D47_SCAP_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT3D47_TestItem_SCapRawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT3D47_FORCE_TOUCH_SCAP_CB_TEST
        if (Code_FT3D47_FORCE_TOUCH_SCAP_CB_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT3D47_TestItem_ForceTouch_SCapCbTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT3D47_FORCE_TOUCH_SCAP_RAWDATA_TEST
        if (Code_FT3D47_FORCE_TOUCH_SCAP_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT3D47_TestItem_ForceTouch_SCapRawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }


    }

    //--------------3. End Part
    FinishTest();

    //--------------4. return result
    return bTestResult;


}

/************************************************************************
* Name: FT3D47_TestItem_EnterFactoryMode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_EnterFactoryMode(void)
{
    unsigned char ReCode = ERROR_CODE_INVALID_PARAM;
    int iRedo = 5;  //If failed, repeat 5 times.
    int i ;
    unsigned char chPattern=0;

    SysDelay(150);
    for (i = 1; i <= iRedo; i++)
    {
        ReCode = EnterFactory();
        if (ERROR_CODE_OK != ReCode)
        {
            FTS_TEST_ERROR("Failed to Enter factory mode...");
            if (i < iRedo)
            {
                SysDelay(50);
                continue;
            }
        }
        else
        {
            break;
        }

    }
    SysDelay(300);


    if (ReCode != ERROR_CODE_OK)
    {
        return ReCode;
    }

    //After the success of the factory model, read the number of channels
    ReCode = GetChannelNum();

    ////////////set FIR��0��close��1��open
    //theDevice.m_cHidDev[m_NumDevice]->WriteReg(0xFB, 0);

    // to determine whether the V3 screen body
    ReCode = ReadReg( REG_PATTERN_3C47, &chPattern );
    if (chPattern == 1)
    {
        m_bV3TP = true;
    }
    else
    {
        m_bV3TP = false;
    }

    return ReCode;
}
/************************************************************************
* Name: FT3D47_TestItem_RawDataTest
* Brief:  TestItem: RawDataTest. Check if MCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_RawDataTest(bool * bTestResult)
{
    unsigned char ReCode = 0;
    bool btmpresult = true;
    int RawDataMin;
    int RawDataMax;
    unsigned char ucFre; /* bakeup the current frequency */

    unsigned char OriginValue = 0xff;
    int index = 0;
    int iRow, iCol;
    int iValue = 0;

    FTS_TEST_DBG("Test for ft3d47");


    FTS_TEST_INFO("\n\n==============================Test Item: -------- Raw Data  Test \n");
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }


    /* Read Current Frequency */
    ReCode = ReadReg( REG_FREQUENCY, &ucFre );

    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n Read  REG_FREQUENCY error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }

    /*
      1:auto normalize rawdata
      0:all normalize rawdata
    */
    ReCode = ReadReg( REG_NORMALIZE_TYPE, &OriginValue );// read the original value
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-2");

        FTS_TEST_ERROR("\n Read  REG_NORMALIZE_TYPE error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }


    /* auto normalize rawdata */
    if (g_ScreenSetParam.isNormalize == Auto_Normalize)
    {

        FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-3");

        /* if original value is not the value needed,write the register to change */
        if (OriginValue != 1)
        {
            ReCode = WriteReg( REG_NORMALIZE_TYPE, 0x01 );

            if (ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_ERROR("\n write  REG_NORMALIZE_TYPE error. Error Code: %d",  ReCode);
                goto TEST_ERR;
            }
        }

        //Set Frequecy High
        FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-4");


        FTS_TEST_DBG( "\n=========Set Frequecy High\n" );
        ReCode = WriteReg( REG_FREQUENCY, 0x81 );
        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("\n Set Frequecy High error. Error Code: %d",  ReCode);
            goto TEST_ERR;
        }

        //change register value before,need to lose 3 frame data
        for (index = 0; index < 3; ++index )
        {
            ReCode = GetRawData();
        }

        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);
            goto TEST_ERR;
        }

        FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-5");

        /* Debug Info */
        ShowRawData();

        ////////////////////////////////To Determine RawData if in Range or not
        for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
        {
            for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
            {
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                RawDataMin = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Min[iRow][iCol];
                RawDataMax = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Max[iRow][iCol];
                iValue = m_RawData[iRow][iCol];
                if (iValue < RawDataMin || iValue > RawDataMax)
                {
                    btmpresult = false;
                    FTS_TEST_DBG("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                 iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
                }
            }
        }

        //////////////////////////////Save Test Data
        Save_Test_Data(m_RawData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 1);
    }
    else
    {
        FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-6");


        if (OriginValue != 0) //if original value is not the value needed,write the register to change
        {
            ReCode = WriteReg( REG_NORMALIZE_TYPE, 0x00 );

            FTS_TEST_INFO("WriteReg( REG_NORMALIZE_TYPE, 0x00 )");

            if (ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_ERROR("\n write REG_NORMALIZE_TYPE error. Error Code: %d",  ReCode);
                goto TEST_ERR;
            }
        }

        ReCode =  ReadReg( REG_FREQUENCY, &ucFre );

        FTS_TEST_INFO("ReadReg( REG_FREQUENCY, &ucFre )");

        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("\n Read frequency error. Error Code: %d",  ReCode);
            goto TEST_ERR;
        }


        //Set Frequecy Low
        if (g_stCfg_FT3D47_BasicThreshold.RawDataTest_SetLowFreq)
        {
            FTS_TEST_DBG("\n=========Set Frequecy Low");
            ReCode = WriteReg( REG_FREQUENCY, 0x80 );
            if (ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_ERROR("\n write frequency error. Error Code: %d",  ReCode);
                goto TEST_ERR;
            }

            SysDelay(100);

            //change register value before,need to lose 3 frame data
            for (index = 0; index < 3; ++index )
            {
                ReCode = GetRawData();
            }

            if ( ReCode != ERROR_CODE_OK )
            {
                FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);
                goto TEST_ERR;
            }

            FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-7");

            ShowRawData();

            ////////////////////////////////To Determine RawData if in Range or not
            for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
            {

                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                    RawDataMin = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Min[iRow][iCol];
                    RawDataMax = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Max[iRow][iCol];
                    iValue = m_RawData[iRow][iCol];
                    if (iValue < RawDataMin || iValue > RawDataMax)
                    {
                        btmpresult = false;
                        FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                       iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
                    }
                }
            }

            //////////////////////////////Save Test Data
            Save_Test_Data(m_RawData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 1);
        }


        //Set Frequecy High
        if ( g_stCfg_FT3D47_BasicThreshold.RawDataTest_SetHighFreq )
        {

            FTS_TEST_DBG( "\n=========Set Frequecy High");
            ReCode = WriteReg( REG_FREQUENCY, 0x81 );
            if (ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_ERROR("\n Set Frequecy High error. Error Code: %d",  ReCode);
                goto TEST_ERR;
            }

            SysDelay(100);

            //change register value before,need to lose 3 frame data
            for (index = 0; index < 3; ++index )
            {
                ReCode = GetRawData();
            }

            if ( ReCode != ERROR_CODE_OK )
            {
                FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);

                if ( ReCode != ERROR_CODE_OK )
                {
                    goto TEST_ERR;
                }
            }

            ShowRawData();

            ////////////////////////////////To Determine RawData if in Range or not
            for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
            {

                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)
                    {
                        continue;//Invalid Node
                    }

                    RawDataMin = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Min[iRow][iCol];
                    RawDataMax = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Max[iRow][iCol];
                    iValue = m_RawData[iRow][iCol];

                    if (iValue < RawDataMin || iValue > RawDataMax)
                    {
                        btmpresult = false;
                        FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                       iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
                    }
                }
            }

            //////////////////////////////Save Test Data
            Save_Test_Data(m_RawData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 2);
        }

    }


    FTS_TEST_INFO("WriteReg( REG_NORMALIZE_TYPE, OriginValue )");

    ReCode = WriteReg( REG_NORMALIZE_TYPE, OriginValue ); /* recovery REG_NORMALIZE_TYPE reg value */
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n Write REG_NORMALIZE_TYPE error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }

    FTS_TEST_INFO("WriteReg( REG_FREQUENCY, ucFre )");

    ReCode = WriteReg( REG_FREQUENCY, ucFre ); /* recovery REG_FREQUENCY reg value */
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n Write REG_FREQUENCY error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }

    TestResultLen += sprintf(TestResult+TestResultLen,"RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    //-------------------------Result
    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//RawData Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//RawData Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_DBG("\n\n//RawData Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen,"RawData Test is NG. \n\n");
    return ReCode;

}
/************************************************************************
* Name: FT3D47_TestItem_SCapRawDataTest
* Brief:  TestItem: SCapRawDataTest. Check if SCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_SCapRawDataTest(bool * bTestResult)
{
    int i =0;
    int RawDataMin = 0;
    int RawDataMax = 0;
    int Value = 0;
    boolean bFlag = true;
    unsigned char ReCode = 0;
    boolean btmpresult = true;
    int iMax=0;
    int iMin=0;
    int iAvg=0;
    int ByteNum=0;
    unsigned char wc_value = 0;//waterproof channel value

    int iCount = 0;
    int ibiggerValue = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -------- Scap RawData Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = ReadReg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to read REG_WATER_CHANNEL_SELECT. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("Failed to Scan SCap RawData! ");
        goto TEST_ERR;
    }
    for (i = 0; i < 3; i++)
    {
        memset(m_iTempRawData, 0, sizeof(m_iTempRawData));

        //Waterproof rawdata
        ByteNum = (m_iAllTx + m_iAllRx)*2;
        ReCode = ReadRawData(0, 0xAC, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("Failed to ReadRawData water! ");
            goto TEST_ERR;
        }

        memcpy( m_RawData[0+g_ScreenSetParam.iTxNum], m_iTempRawData, sizeof(int)*g_ScreenSetParam.iRxNum );
        memcpy( m_RawData[1+g_ScreenSetParam.iTxNum], m_iTempRawData + m_iAllRx, sizeof(int)*g_ScreenSetParam.iTxNum );

        // No waterproof rawdata
        ByteNum = (m_iAllTx + m_iAllRx)*2;
        ReCode = ReadRawData(0, 0xAB, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("Failed to ReadRawData no water! ");
            goto TEST_ERR;
        }
        memcpy( m_RawData[2+g_ScreenSetParam.iTxNum], m_iTempRawData, sizeof(int)*g_ScreenSetParam.iRxNum );
        memcpy( m_RawData[3+g_ScreenSetParam.iTxNum], m_iTempRawData + m_iAllRx, sizeof(int)*g_ScreenSetParam.iTxNum );
    }


    //-----3. Judge

    //Waterproof ON
    bFlag=GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_SetWaterproof_ON && bFlag )
    {
        iCount = 0;
        RawDataMin = g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_ON_Min;
        RawDataMax = g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_ON_Max;
        iMax = -m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;


        bFlag=GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-ON:");
        for ( i = 0; bFlag && i < g_ScreenSetParam.iRxNum; i++ )
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Min[0][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Max[0][i];
            Value = m_RawData[0+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-ON:");
        for (i = 0; bFlag && i < g_ScreenSetParam.iTxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Min[1][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Max[1][i];
            Value = m_RawData[1+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap RawData in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+0, 2, ibiggerValue, 1);
    }

    //Waterproof OFF
    bFlag=GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_SetWaterproof_OFF && bFlag)
    {
        iCount = 0;
        RawDataMin = g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_OFF_Min;
        RawDataMax = g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_OFF_Max;
        iMax = -m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;

        bFlag=GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-OFF:");
        for (i = 0; bFlag && i < g_ScreenSetParam.iRxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Min[0][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Max[0][i];
            Value = m_RawData[2+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            //FTS_TEST_DBG("zaxzax3 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            //strTemp += str;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }

        bFlag=GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-OFF:");
        for (i = 0; bFlag && i < g_ScreenSetParam.iTxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;

            Value = m_RawData[3+g_ScreenSetParam.iTxNum][i];
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Min[1][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Max[1][i];
            //FTS_TEST_DBG("zaxzax4 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap RawData in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+2, 2, ibiggerValue, 2);
    }

    TestResultLen += sprintf(TestResult+TestResultLen,"SCap RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    //-----5. Test Result
    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//SCap RawData Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//SCap RawData Test is NG!");
    }
    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_INFO("\n\n//SCap RawData Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen,"SCap RawData Test is NG. \n\n");
    return ReCode;
}

/************************************************************************
* Name: FT3D47_TestItem_SCapCbTest
* Brief:  TestItem: SCapCbTest. Check if SCAP Cb is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_SCapCbTest(bool* bTestResult)
{
    int i,/* j, iOutNum,*/index,Value,CBMin,CBMax;
    boolean bFlag = true;
    unsigned char ReCode;
    boolean btmpresult = true;
    int iMax, iMin, iAvg;
    unsigned char wc_value = 0;

    int iCount = 0;
    int ibiggerValue = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -----  Scap CB Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = ReadReg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n Read REG_WATER_CHANNEL_SELECT error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("Failed to Scan SCap RawData!ReCode = %d. ",  ReCode);
        goto TEST_ERR;
    }


    for (i = 0; i < 3; i++)
    {
        memset(m_RawData, 0, sizeof(m_RawData));
        memset(m_ucTempData, 0, sizeof(m_ucTempData));

        //Waterproof CB
        ReCode = WriteReg( REG_ScWorkMode, 1 );//ScWorkMode:  1: waterproof 0:Non-waterproof
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Get REG_ScWorkMode Failed!");
            goto TEST_ERR;
        }

        ReCode = StartScan();
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("StartScan Failed!");
            goto TEST_ERR;
        }

        ReCode = WriteReg( REG_ScCbAddrR, 0 );
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Write REG_ScCbAddrR Failed!");
            goto TEST_ERR;
        }

        ReCode = GetTxSC_CB( m_iAllTx + m_iAllRx + 128, m_ucTempData );
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("GetTxSC_CB Failed!");
            goto TEST_ERR;
        }

        for ( index = 0; index < g_ScreenSetParam.iRxNum; ++index )
        {
            m_RawData[0 + g_ScreenSetParam.iTxNum][index]= m_ucTempData[index];
        }
        for ( index = 0; index < g_ScreenSetParam.iTxNum; ++index )
        {
            m_RawData[1 + g_ScreenSetParam.iTxNum][index] = m_ucTempData[index + m_iAllRx];
        }

        // Non Waterproof rawdata
        ReCode = WriteReg( REG_ScWorkMode, 0 );//ScWorkMode:  1: waterproof 0:Non-waterproof
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Get REG_ScWorkMode Failed!");
            goto TEST_ERR;
        }

        ReCode = StartScan();
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("StartScan Failed!");
            goto TEST_ERR;
        }

        ReCode = WriteReg( REG_ScCbAddrR, 0 );
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Write REG_ScCbAddrR Failed!");
            goto TEST_ERR;
        }

        ReCode = GetTxSC_CB( m_iAllTx + m_iAllRx + 128, m_ucTempData );
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("GetTxSC_CB Failed!");
            goto TEST_ERR;
        }
        for ( index = 0; index < g_ScreenSetParam.iRxNum; ++index )
        {
            m_RawData[2 + g_ScreenSetParam.iTxNum][index]= m_ucTempData[index];
        }
        for ( index = 0; index < g_ScreenSetParam.iTxNum; ++index )
        {
            m_RawData[3 + g_ScreenSetParam.iTxNum][index] = m_ucTempData[index + m_iAllRx];
        }

        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Failed to Get SCap CB!");
        }
    }

    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //-----3. Judge

    //Waterproof ON
    bFlag=GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.SCapCbTest_SetWaterproof_ON && bFlag)
    {
        FTS_TEST_DBG("SCapCbTest in WaterProof On Mode:  ");

        iMax = -m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;


        bFlag=GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Rx:  ");
        for ( i = 0; bFlag && i < g_ScreenSetParam.iRxNum; i++ )
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Min[0][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Max[0][i];
            Value = m_RawData[0+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value; //find the Max Value
            if (iMin > Value) iMin = Value; //find the Min Value
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Tx:  ");
        for (i = 0; bFlag &&  i < g_ScreenSetParam.iTxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Min[1][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Max[1][i];
            Value = m_RawData[1+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap CB in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+0, 2, ibiggerValue, 1);
    }

    bFlag=GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.SCapCbTest_SetWaterproof_OFF && bFlag)
    {
        FTS_TEST_DBG("SCapCbTest in WaterProof OFF Mode:  ");
        iMax = -m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;


        bFlag=GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Rx:  ");
        for (i = 0; bFlag &&  i < g_ScreenSetParam.iRxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Min[0][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Max[0][i];
            Value = m_RawData[2+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Tx:  ");
        for (i = 0; bFlag && i < g_ScreenSetParam.iTxNum; i++)
        {
            //if( m_ScapInvalide[1][i] == 0 )      continue;
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Min[1][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Max[1][i];
            Value = m_RawData[3+g_ScreenSetParam.iTxNum][i];

            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap CB in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+2, 2, ibiggerValue, 2);
    }

    //-----5. Test Result

    TestResultLen += sprintf(TestResult+TestResultLen,"SCap CB Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//SCap CB Test Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//SCap CB Test Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("\n\n//SCap CB Test Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen,"SCap CB Test is NG. \n\n");
    return ReCode;
}



/************************************************************************
* Name: FT3D47_TestItem_ForceTouch_SCapRawDataTest
* Brief:  TestItem: ForceTouch_SCapRawDataTest. Check if ForceTouch_SCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_ForceTouch_SCapRawDataTest(bool * bTestResult)
{
    int m = 0;
    int i =0;
    int RawDataMin = 0;
    int RawDataMax = 0;
    int Value = 0;
    boolean bFlag = true;
    unsigned char ReCode = 0;
    boolean btmpresult = true;
    int iMax=0;
    int iMin=0;
    int iAvg=0;
    int ByteNum=0;
    unsigned char wc_value = 0;//waterproof channel value
    int iCount = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -------- Force Touch Scap RawData Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = ReadReg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to read REG_WATER_CHANNEL_SELECT. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("Failed to Scan SCap RawData! ");
        goto TEST_ERR;
    }
    for (i = 0; i < 3; i++)
    {
        memset(m_iTempRawData, 0, sizeof(m_iTempRawData));

        //Waterproof rawdata
        ByteNum = (g_ScreenSetParam.iRxNum+g_ScreenSetParam.iTxNum + m_iForceTouchRx + m_iForceTouchTx)*2;
        ReCode = ReadRawData(0, 0xAC, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("Failed to ReadRawData water! ");
            goto TEST_ERR;
        }

        for (m = 0; m < m_iForceTouchTx; m++)
        {
            m_Force_RawData_Water[0][m] = m_iTempRawData[g_ScreenSetParam.iRxNum+g_ScreenSetParam.iTxNum + m];
        }

        FTS_TEST_INFO("g_ScreenSetParam.iTxNum[%d]", g_ScreenSetParam.iTxNum);
        FTS_TEST_INFO("g_ScreenSetParam.iRxNum[%d]", g_ScreenSetParam.iRxNum);
        FTS_TEST_INFO("m_iForceTouchRx[%d]", m_iForceTouchRx);
        FTS_TEST_INFO("m_iForceTouchTx[%d]", m_iForceTouchTx);

        //No Waterproof rawdata
        //ByteNum = (g_ScreenSetParam.iTxNum + g_ScreenSetParam.iRxNum)*2;
        ByteNum = (g_ScreenSetParam.iRxNum+g_ScreenSetParam.iTxNum + m_iForceTouchRx + m_iForceTouchTx)*2;
        ReCode = ReadRawData(0, 0xAB, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("Failed to ReadRawData no water! ");
            goto TEST_ERR;
        }

        for (m = 0; m < m_iForceTouchTx; m++)
        {
            m_Force_RawData_Normal[0][m] = m_iTempRawData[g_ScreenSetParam.iRxNum+g_ScreenSetParam.iTxNum + m];
        }

        FTS_TEST_INFO("g_ScreenSetParam.iTxNum[%d]", g_ScreenSetParam.iTxNum);
        FTS_TEST_INFO("g_ScreenSetParam.iRxNum[%d]", g_ScreenSetParam.iRxNum);
        FTS_TEST_INFO("m_iForceTouchRx[%d]", m_iForceTouchRx);
        FTS_TEST_INFO("m_iForceTouchTx[%d]", m_iForceTouchTx);
    }


    //-----3. Judge

    //Waterproof ON
    bFlag=GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_SetWaterproof_ON && bFlag )
    {
        iCount = 0;
        RawDataMin = g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_ON_Min;
        RawDataMax = g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_ON_Max;
        iMax = -m_Force_RawData_Water[0][0];
        iMin = 2 * m_Force_RawData_Water[0][0];
        iAvg = 0;
        Value = 0;


        bFlag=GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-ON:");
        for ( i = 0; bFlag && i < m_iForceTouchRx; i++ )
        {
            //if( g_stCfg_MCap_DetailThreshold.ForceTouch_InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapRawDataTest_ON_Min[0][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapRawDataTest_ON_Max[0][i];
            Value = m_Force_RawData_Water[0][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-ON:");
        for (i = 0; bFlag && i < m_iForceTouchTx; i++)
        {
            //if( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapRawDataTest_ON_Min[0][i+m_iForceTouchRx];
            RawDataMax = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapRawDataTest_ON_Max[0][i+m_iForceTouchRx];
            Value = m_Force_RawData_Water[0][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("Force Touch SCap RawData in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
    }

    //Waterproof OFF
    bFlag=GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_SetWaterproof_OFF && bFlag)
    {
        iCount = 0;
        RawDataMin = g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_OFF_Min;
        RawDataMax = g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_OFF_Max;
        iMax = -m_Force_RawData_Normal[0][0];
        iMin = 2 * m_Force_RawData_Normal[0][0];
        iAvg = 0;
        Value = 0;

        bFlag=GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-OFF:");
        for (i = 0; bFlag && i < m_iForceTouchRx; i++)
        {
            //if( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapRawDataTest_OFF_Min[0][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapRawDataTest_OFF_Max[0][i];
            Value = m_Force_RawData_Normal[0][i];
            iAvg += Value;

            //FTS_TEST_DBG("zaxzax3 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            //strTemp += str;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }

        bFlag=GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-OFF:");
        for (i = 0; bFlag && i < m_iForceTouchTx; i++)
        {
            //if( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;

            Value = m_Force_RawData_Normal[0][i];
            RawDataMin = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapRawDataTest_OFF_Min[0][i+m_iForceTouchRx];
            RawDataMax = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapRawDataTest_OFF_Max[0][i+m_iForceTouchRx];
            //FTS_TEST_DBG("zaxzax4 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("Force Touch SCap RawData in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
    }

    Save_Test_Data(m_Force_RawData_Water, 0, 1, m_iForceTouchTx, 1);
    Save_Test_Data(m_Force_RawData_Normal, 0, 1, m_iForceTouchTx, 1);

    //-----5. Test Result

    TestResultLen += sprintf(TestResult+TestResultLen,"Force Touch SCap RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//Force Touch SCap RawData Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//Force Touch SCap RawData Test is NG!");
    }
    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_INFO("\n\n//Force Touch SCap RawData Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen,"Force Touch SCap RawData Test is NG. \n\n");
    return ReCode;
}


/************************************************************************
* Name: FT3D47_TestItem_ForceTouch_SCapCbTest
* Brief:  TestItem: ForceTouch_SCapCbTest. Check if ForceTouch_SCAP Cb is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_ForceTouch_SCapCbTest(bool* bTestResult)
{
    int i,/* j, iOutNum,*/index,Value,CBMin,CBMax;
    boolean bFlag = true;
    unsigned char ReCode;
    boolean btmpresult = true;
    int iMax, iMin, iAvg;
    unsigned char wc_value = 0;
    int iCount = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -----  Force Touch Scap CB Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = ReadReg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n Read REG_WATER_CHANNEL_SELECT error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("Failed to Scan SCap RawData!ReCode = %d. ",  ReCode);
        goto TEST_ERR;
    }


    for (i = 0; i < 3; i++)
    {
        memset(m_RawData, 0, sizeof(m_RawData));
        memset(m_ucTempData, 0, sizeof(m_ucTempData));

        //Waterproof CB
        ReCode = WriteReg( REG_ScWorkMode, 1 );//ScWorkMode:  1: waterproof 0:Non-waterproof
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Get REG_ScWorkMode Failed!");
            goto TEST_ERR;
        }

        ReCode = StartScan();
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("StartScan Failed!");
            goto TEST_ERR;
        }

        ReCode = WriteReg( REG_ScCbAddrR, 0 );
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Write REG_ScCbAddrR Failed!");
            goto TEST_ERR;
        }

        ReCode = GetTxSC_CB( m_iAllTx + m_iAllRx + 128, m_ucTempData );
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("GetTxSC_CB Failed!");
            goto TEST_ERR;
        }

        FTS_TEST_INFO("m_iAllTx[%d], m_iAllRx[%d]", m_iAllTx, m_iAllRx);
        index = g_ScreenSetParam.iRxNum + g_ScreenSetParam.iTxNum;

        for (i = 0; i < m_iForceTouchTx; i++)
        {

            m_Force_CBData_Water[0][i] = m_ucTempData[index];
            FTS_TEST_DBG("i[%d], CbData[%d]", i , m_Force_CBData_Water[0][i]);
            index++;
        }

        //No Waterproof rawdata
        ReCode = WriteReg( REG_ScWorkMode, 0 );//ScWorkMode:  1: waterproof 0:Non-waterproof
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Get REG_ScWorkMode Failed!");
            goto TEST_ERR;
        }

        ReCode = StartScan();
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("StartScan Failed!");
            goto TEST_ERR;
        }

        ReCode = WriteReg( REG_ScCbAddrR, 0 );
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Write REG_ScCbAddrR Failed!");
            goto TEST_ERR;
        }

        ReCode = GetTxSC_CB( m_iAllTx + m_iAllRx + 128, m_ucTempData );
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("GetTxSC_CB Failed!");
            goto TEST_ERR;
        }

        index = g_ScreenSetParam.iRxNum + g_ScreenSetParam.iTxNum;

        for (i = 0; i < m_iForceTouchTx; i++)
        {

            m_Force_CBData_Normal[0][i] = m_ucTempData[index];
            FTS_TEST_DBG("i[%d], CbData[%d]", i , m_RawData[0 + g_ScreenSetParam.iTxNum][m_iForceTouchRx+i]);
            index++;
        }

        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Failed to Get SCap CB!");
        }
    }

    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //-----3. Judge

    //Waterproof ON
    bFlag=GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapCBTest_SetWaterproof_ON && bFlag)
    {
        FTS_TEST_DBG("Force Touch SCapCbTest in WaterProof On Mode:  ");

        iMax = -m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;


        bFlag=GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Force Touch SCap CB_Rx:  ");
        for ( i = 0; bFlag && i < m_iForceTouchRx; i++ )
        {
            //if( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapCbTest_ON_Min[0][i];
            CBMax = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapCbTest_ON_Max[0][i];
            Value = m_RawData[0+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value; //find the Max Value
            if (iMin > Value) iMin = Value; //find the Min Value
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Force Touch SCap CB_Tx:  ");
        for (i = 0; bFlag &&  i < m_iForceTouchTx; i++)
        {
            //if( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapCbTest_ON_Min[0][i+m_iForceTouchRx];
            CBMax = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapCbTest_ON_Max[0][i+m_iForceTouchRx];
            Value = m_RawData[1+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            FTS_TEST_DBG("Value = %d.", Value);
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("Force Touch SCap CB in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //ibiggerValue = m_iForceTouchTx>m_iForceTouchRx?m_iForceTouchTx:m_iForceTouchRx;
        //Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+0, 1, m_iForceTouchRx + m_iForceTouchTx, 1);
    }

    bFlag=GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapCBTest_SetWaterproof_OFF && bFlag)
    {
        FTS_TEST_DBG("Force Touch SCapCbTest in WaterProof OFF Mode:  ");
        iMax = -m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;

        bFlag=GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Force Touch SCap CB_Rx:  ");
        for (i = 0; bFlag &&  i < m_iForceTouchRx; i++)
        {
            //if( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapCbTest_OFF_Min[0][i];
            CBMax = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapCbTest_OFF_Max[0][i];
            Value = m_RawData[2+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Force Touch SCap CB_Tx:  ");
        for (i = 0; bFlag && i < m_iForceTouchTx; i++)
        {
            //if( m_ScapInvalide[1][i] == 0 )      continue;
            //if( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapCbTest_OFF_Min[0][i+m_iForceTouchRx];
            CBMax = g_stCfg_MCap_DetailThreshold.ForceTouch_SCapCbTest_OFF_Max[0][i+m_iForceTouchRx];
            Value = m_RawData[3+g_ScreenSetParam.iTxNum][i];

            FTS_TEST_DBG("Value = %d. ", Value);


            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("Force Touch SCap CB in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
//      ibiggerValue = m_iForceTouchTx>m_iForceTouchRx?m_iForceTouchTx:m_iForceTouchRx;
        //Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+2, 1, m_iForceTouchRx + m_iForceTouchTx, 2);
    }

    Save_Test_Data(m_Force_CBData_Water, 0, 1, m_iForceTouchTx, 2);
    Save_Test_Data(m_Force_CBData_Normal, 0, 1, m_iForceTouchTx, 2);

    //-----5. Test Result

    TestResultLen += sprintf(TestResult+TestResultLen,"Force Touch SCap CB Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//Force Touch SCap CB Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//Force Touch SCap CB Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("\n\n//Force Touch SCap CB Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen,"Force Touch SCap CB Test is NG. \n\n");
    return ReCode;
}


/************************************************************************
* Name: FT3C47_get_test_data
* Brief:  get data of test result
* Input: none
* Output: pTestData, the returned buff
* Return: the length of test data. if length > 0, got data;else ERR.
***********************************************************************/
int FT3D47_get_test_data(char *pTestData)
{
    if (NULL == pTestData)
    {
        FTS_TEST_DBG("[focal] %s pTestData == NULL ",  __func__);
        return -1;
    }
    memcpy(pTestData, g_pStoreAllData, (g_lenStoreMsgArea+g_lenStoreDataArea));
    return (g_lenStoreMsgArea+g_lenStoreDataArea);
}


/************************************************************************
* Name: GetPanelRows(Same function name as FT_MultipleTest)
* Brief:  Get row of TP
* Input: none
* Output: pPanelRows
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetPanelRows(unsigned char *pPanelRows)
{
    return ReadReg(REG_TX_NUM, pPanelRows);
}

/************************************************************************
* Name: GetPanelCols(Same function name as FT_MultipleTest)
* Brief:  get column of TP
* Input: none
* Output: pPanelCols
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetPanelCols(unsigned char *pPanelCols)
{
    return ReadReg(REG_RX_NUM, pPanelCols);
}
/************************************************************************
* Name: StartScan(Same function name as FT_MultipleTest)
* Brief:  Scan TP, do it before read Raw Data
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int StartScan(void)
{
    unsigned char RegVal = 0;
    unsigned char times = 0;
    const unsigned char MaxTimes = 250;
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;

    ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);
    if (ReCode == ERROR_CODE_OK)
    {
        RegVal |= 0x80;     //Top bit position 1, start scan
        ReCode = WriteReg(DEVIDE_MODE_ADDR, RegVal);
        if (ReCode == ERROR_CODE_OK)
        {
            while (times++ < MaxTimes)      //Wait for the scan to complete
            {
                SysDelay(8);    //8ms
                ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);
                if (ReCode == ERROR_CODE_OK)
                {
                    if ((RegVal>>7) == 0)    break;
                }
                else
                {
                    FTS_TEST_ERROR("StartScan read DEVIDE_MODE_ADDR error.");
                    break;
                }
            }
            if (times < MaxTimes)    ReCode = ERROR_CODE_OK;
            else ReCode = ERROR_CODE_COMM_ERROR;
        }
        else
            FTS_TEST_ERROR("StartScan write DEVIDE_MODE_ADDR error.");
    }
    else
        FTS_TEST_ERROR("StartScan read DEVIDE_MODE_ADDR error.");
    return ReCode;

}
/************************************************************************
* Name: ReadRawData(Same function name as FT_MultipleTest)
* Brief:  read Raw Data
* Input: Freq(No longer used, reserved), LineNum, ByteNum
* Output: pRevBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer)
{
    unsigned char ReCode=ERROR_CODE_COMM_ERROR;
    unsigned char I2C_wBuffer[3];
    int i, iReadNum;
    unsigned short BytesNumInTestMode1=0;

    iReadNum=ByteNum/BYTES_PER_TIME;

    if (0 != (ByteNum%BYTES_PER_TIME)) iReadNum++;

    if (ByteNum <= BYTES_PER_TIME)
    {
        BytesNumInTestMode1 = ByteNum;
    }
    else
    {
        BytesNumInTestMode1 = BYTES_PER_TIME;
    }

    ReCode = WriteReg(REG_LINE_NUM, LineNum);//Set row addr;

    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("Failed to write REG_LINE_NUM! ");
        goto READ_ERR;
    }

    //***********************************************************Read raw data
    I2C_wBuffer[0] = REG_RawBuf0;   //set begin address
    if (ReCode == ERROR_CODE_OK)
    {
        focal_msleep(10);
        ReCode = Comm_Base_IIC_IO(I2C_wBuffer, 1, m_ucTempData, BytesNumInTestMode1);
        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("read rawdata Comm_Base_IIC_IO Failed!1 ");
            goto READ_ERR;
        }
    }

    for (i=1; i<iReadNum; i++)
    {
        if (ReCode != ERROR_CODE_OK) break;

        if (i==iReadNum-1) //last packet
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, m_ucTempData+BYTES_PER_TIME*i, ByteNum-BYTES_PER_TIME*i);
            if (ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_ERROR("read rawdata Comm_Base_IIC_IO Failed!2 ");
                goto READ_ERR;
            }
        }
        else
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, m_ucTempData+BYTES_PER_TIME*i, BYTES_PER_TIME);

            if (ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_ERROR("read rawdata Comm_Base_IIC_IO Failed!3 ");
                goto READ_ERR;
            }
        }

    }

    if (ReCode == ERROR_CODE_OK)
    {
        for (i=0; i<(ByteNum>>1); i++)
        {
            pRevBuffer[i] = (m_ucTempData[i<<1]<<8)+m_ucTempData[(i<<1)+1];
            //if(pRevBuffer[i] & 0x8000)//have sign bit
            //{
            //  pRevBuffer[i] -= 0xffff + 1;
            //}
        }
    }

READ_ERR:
    return ReCode;

}
/************************************************************************
* Name: GetTxSC_CB(Same function name as FT_MultipleTest)
* Brief:  get CB of Tx SCap
* Input: index
* Output: pcbValue
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char GetTxSC_CB(unsigned char index, unsigned char *pcbValue)
{
    unsigned char ReCode = ERROR_CODE_OK;
    unsigned char wBuffer[4];

    if (index<128) //single read
    {
        *pcbValue = 0;
        WriteReg(REG_ScCbAddrR, index);
        ReCode = ReadReg(REG_ScCbBuf0, pcbValue);
    }
    else//Sequential Read length index-128
    {
        WriteReg(REG_ScCbAddrR, 0);
        wBuffer[0] = REG_ScCbBuf0;
        ReCode = Comm_Base_IIC_IO(wBuffer, 1, pcbValue, index-128);

    }

    return ReCode;
}


/************************************************************************
* Name: Save_Test_Data
* Brief:  Storage format of test data
* Input: int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount
* Output: none
* Return: none
***********************************************************************/
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount)
{
    int iLen = 0;
    int i = 0, j = 0;

    //Save  Msg (ItemCode is enough, ItemName is not necessary, so set it to "NA".)
    iLen= sprintf(g_pTmpBuff,"NA, %d, %d, %d, %d, %d, ", \
                  m_ucTestItemCode, Row, Col, m_iStartLine, ItemCount);
    memcpy(g_pMsgAreaLine2+g_lenMsgAreaLine2, g_pTmpBuff, iLen);
    g_lenMsgAreaLine2 += iLen;

    m_iStartLine += Row;
    m_iTestDataCount++;

    //Save Data
    for (i = 0+iArrayIndex; i < Row+iArrayIndex; i++)
    {
        for (j = 0; j < Col; j++)
        {
            if (j == (Col -1)) //The Last Data of the Row, add "\n"
                iLen= sprintf(g_pTmpBuff,"%d, \n ",  iData[i][j]);
            else
                iLen= sprintf(g_pTmpBuff,"%d, ", iData[i][j]);

            memcpy(g_pStoreDataArea+g_lenStoreDataArea, g_pTmpBuff, iLen);
            g_lenStoreDataArea += iLen;
        }
    }

}

/************************************************************************
* Name: GetChannelNum
* Brief:  Get Channel Num(Tx and Rx)
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetChannelNum(void)
{
    unsigned char ReCode=0;
    unsigned char rBuffer[1]= {0}; //= new unsigned char;

    unsigned char TxNum=0;
    unsigned char RxNum=0;

    //m_strCurrentTestMsg = "Get Tx Num...";
    ReCode = GetPanelRows(rBuffer);
    if (ReCode == ERROR_CODE_OK)
    {
        g_ScreenSetParam.iTxNum = rBuffer[0];
        if (g_ScreenSetParam.iTxNum > g_ScreenSetParam.iUsedMaxTxNum)
        {

            FTS_TEST_ERROR("Failed to get Tx number, Get num = %d, UsedMaxNum = %d",
                           g_ScreenSetParam.iTxNum, g_ScreenSetParam.iUsedMaxTxNum);
            g_ScreenSetParam.iTxNum = 0;
            return ERROR_CODE_INVALID_PARAM;
        }
    }
    else
    {
        FTS_TEST_ERROR("Failed to get Tx number");
    }

    ///////////////m_strCurrentTestMsg = "Get Rx Num...";

    ReCode = GetPanelCols(rBuffer);
    if (ReCode == ERROR_CODE_OK)
    {
        g_ScreenSetParam.iRxNum = rBuffer[0];
        if (g_ScreenSetParam.iRxNum > g_ScreenSetParam.iUsedMaxRxNum)
        {

            FTS_TEST_ERROR("Failed to get Rx number, Get num = %d, UsedMaxNum = %d",
                           g_ScreenSetParam.iRxNum, g_ScreenSetParam.iUsedMaxRxNum);
            g_ScreenSetParam.iRxNum = 0;
            return ERROR_CODE_INVALID_PARAM;
        }
    }
    else
    {
        FTS_TEST_ERROR("Failed to get Rx number");
    }


    //get force touch channel num
    //m_strCurrentTestMsg = "Get real Rx Num...";
    ReCode = ReadReg(REG_REAL_RX_NUM,&RxNum);
    if (ReCode == ERROR_CODE_OK)
    {
        m_iForceTouchRx = RxNum - g_ScreenSetParam.iRxNum;
    }
    else
    {
        FTS_TEST_DBG("Failed to get real Rx number\r");
    }

    //m_strCurrentTestMsg = "Get real Tx Num...";
    ReCode = ReadReg(REG_REAL_TX_NUM,&TxNum);
    if (ReCode == ERROR_CODE_OK)
    {
        m_iForceTouchTx = TxNum - g_ScreenSetParam.iTxNum;
    }
    else
    {
        FTS_TEST_ERROR("Failed to get real Tx number\r");
    }

    m_iAllTx = TxNum;
    m_iAllRx = RxNum;

    FTS_TEST_DBG("m_iForceTouchRx = %d, m_iForceTouchTx = %d", m_iForceTouchRx, m_iForceTouchTx);
    return ReCode;

}
/************************************************************************
* Name: GetRawData
* Brief:  Get Raw Data of MCAP
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetRawData(void)
{
    unsigned char ReCode = ERROR_CODE_OK;
    int iRow = 0;
    int iCol = 0;

    //--------------------------------------------Enter Factory Mode
    ReCode = EnterFactory();
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_ERROR("Failed to Enter Factory Mode...");
        return ReCode;
    }


    //--------------------------------------------Check Num of Channel
    if (0 == (g_ScreenSetParam.iTxNum + g_ScreenSetParam.iRxNum))
    {
        ReCode = GetChannelNum();
        if ( ERROR_CODE_OK != ReCode )
        {
            FTS_TEST_ERROR("Error Channel Num...");
            return ERROR_CODE_INVALID_PARAM;
        }
    }

    //--------------------------------------------Start Scanning
    FTS_TEST_DBG("Start Scan ...");
    ReCode = StartScan();
    if (ERROR_CODE_OK != ReCode)
    {
        FTS_TEST_ERROR("Failed to Scan ...");
        return ReCode;
    }

    //--------------------------------------------Read RawData, Only MCAP
    memset(m_RawData, 0, sizeof(m_RawData));
    ReCode = ReadRawData( 1, 0xAA, ( g_ScreenSetParam.iTxNum * g_ScreenSetParam.iRxNum )*2, m_iTempRawData );
    for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
    {
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            m_RawData[iRow][iCol] = m_iTempRawData[iRow*g_ScreenSetParam.iRxNum + iCol];
        }
    }
    return ReCode;
}
/************************************************************************
* Name: ShowRawData
* Brief:  Show RawData
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static void ShowRawData(void)
{
    int iRow, iCol;
    //----------------------------------------------------------Show RawData
    for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
    {
        FTS_TEST_DBG("Tx%2d:  ", iRow+1);
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            FTS_TEST_DBG("%5d    ", m_RawData[iRow][iCol]);
        }
        FTS_TEST_DBG("\n ");
    }
}

/************************************************************************
* Name: GetTestCondition
* Brief:  Check whether Rx or TX need to test, in Waterproof ON/OFF Mode.
* Input: none
* Output: none
* Return: true: need to test; false: Not tested.
***********************************************************************/
static boolean GetTestCondition(int iTestType, unsigned char ucChannelValue)
{
    boolean bIsNeeded = false;
    switch (iTestType)
    {
        case WT_NeedProofOnTest://Bit5:  0:test waterProof mode ;  1 not test waterProof mode
            bIsNeeded = !( ucChannelValue & 0x20 );
            break;
        case WT_NeedProofOffTest://Bit7: 0: test normal mode  1:not test normal mode
            bIsNeeded = !( ucChannelValue & 0x80 );
            break;
        case WT_NeedTxOnVal:
            //Bit6:  0 : test waterProof Rx+Tx  1:test waterProof single channel
            //Bit2:  0: test waterProof Tx only;  1:  test waterProof Rx only
            bIsNeeded = !( ucChannelValue & 0x40 ) || !( ucChannelValue & 0x04 );
            break;
        case WT_NeedRxOnVal:
            //Bit6:  0 : test waterProof Rx+Tx  1 test waterProof single channel
            //Bit2:  0: test waterProof Tx only;  1:  test waterProof Rx only
            bIsNeeded = !( ucChannelValue & 0x40 ) || ( ucChannelValue & 0x04 );
            break;
        case WT_NeedTxOffVal://Bit1,Bit0:  00:test normal Tx; 10: test normal Rx+Tx
            bIsNeeded = (0x00 == (ucChannelValue & 0x03)) || (0x02 == ( ucChannelValue & 0x03 ));
            break;
        case WT_NeedRxOffVal://Bit1,Bit0:  01: test normal Rx;    10: test normal Rx+Tx
            bIsNeeded = (0x01 == (ucChannelValue & 0x03)) || (0x02 == ( ucChannelValue & 0x03 ));
            break;
        default:
            break;
    }
    return bIsNeeded;
}

#endif

