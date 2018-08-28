/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: Focaltech_test_config_ftE716.c
*
* Author: Software Development Team, AE
*
* Created: 2016-08-01
*
* Abstract: Set Config for FTE716
*
************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>

#include "../include/focaltech_test_ini.h"
#include "../include/focaltech_test_supported_ic.h"
#include "../focaltech_test_config.h"

#if (FTS_CHIP_TEST_TYPE ==FTE716_TEST)


struct stCfg_FTE716_TestItem g_stCfg_FTE716_TestItem;
struct stCfg_FTE716_BasicThreshold g_stCfg_FTE716_BasicThreshold;

void OnInit_FTE716_TestItem(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();


    //////////////////////////////////////////////////////////// FW Version
    GetPrivateProfileString("TestItem","FW_VERSION_TEST","0",str,strIniFile);
    g_stCfg_FTE716_TestItem.FW_VERSION_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// Factory ID
    GetPrivateProfileString("TestItem","FACTORY_ID_TEST","0",str,strIniFile);
    g_stCfg_FTE716_TestItem.FACTORY_ID_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// Project Code Test
    GetPrivateProfileString("TestItem","PROJECT_CODE_TEST","0",str,strIniFile);
    g_stCfg_FTE716_TestItem.PROJECT_CODE_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// IC Version
    GetPrivateProfileString("TestItem","IC_VERSION_TEST","0",str,strIniFile);
    g_stCfg_FTE716_TestItem.IC_VERSION_TEST = fts_atoi(str);

    /////////////////////////////////// RawData Test
    GetPrivateProfileString("TestItem","RAWDATA_TEST","1",str,strIniFile);
    g_stCfg_FTE716_TestItem.RAWDATA_TEST = fts_atoi(str);

    /////////////////////////////////// CHANNEL_NUM_TEST
    GetPrivateProfileString("TestItem","CHANNEL_NUM_TEST","1",str,strIniFile);
    g_stCfg_FTE716_TestItem.CHANNEL_NUM_TEST = fts_atoi(str);

    /////////////////////////////////// INT_PIN_TEST
    GetPrivateProfileString("TestItem","INT_PIN_TEST","0",str,strIniFile);
    g_stCfg_FTE716_TestItem.INT_PIN_TEST = fts_atoi(str);

    /////////////////////////////////// RESET_PIN_TEST
    GetPrivateProfileString("TestItem","RESET_PIN_TEST","0",str,strIniFile);
    g_stCfg_FTE716_TestItem.RESET_PIN_TEST = fts_atoi(str);

    /////////////////////////////////// NOISE_TEST
    GetPrivateProfileString("TestItem","NOISE_TEST","0",str,strIniFile);
    g_stCfg_FTE716_TestItem.NOISE_TEST = fts_atoi(str);

    /////////////////////////////////// CB_TEST
    GetPrivateProfileString("TestItem","CB_TEST","1",str,strIniFile);
    g_stCfg_FTE716_TestItem.CB_TEST = fts_atoi(str);

    /////////////////////////////////// SHORT_CIRCUIT_TEST
    GetPrivateProfileString("TestItem","SHORT_CIRCUIT_TEST","1",str,strIniFile);
    g_stCfg_FTE716_TestItem.SHORT_CIRCUIT_TEST = fts_atoi(str);

    /////////////////////////////////// OPEN_TEST
    GetPrivateProfileString("TestItem","OPEN_TEST","0",str,strIniFile);
    g_stCfg_FTE716_TestItem.OPEN_TEST = fts_atoi(str);

    /////////////////////////////////// CB_UNIFORMITY_TEST
    GetPrivateProfileString("TestItem","CB_UNIFORMITY_TEST","1",str,strIniFile);
    g_stCfg_FTE716_TestItem.CB_UNIFORMITY_TEST = fts_atoi(str);

    /////////////////////////////////// DIFFER_UNIFORMITY_TEST
    GetPrivateProfileString("TestItem","DIFFER_UNIFORMITY_TEST","1",str,strIniFile);
    g_stCfg_FTE716_TestItem.DIFFER_UNIFORMITY_TEST = fts_atoi(str);

    /////////////////////////////////// DIFFER2_UNIFORMITY_TEST
    GetPrivateProfileString("TestItem","DIFFER2_UNIFORMITY_TEST","0",str,strIniFile);
    g_stCfg_FTE716_TestItem.DIFFER2_UNIFORMITY_TEST = fts_atoi(str);

    /////////////////////////////////// LCD_NOISE_TEST
    GetPrivateProfileString("TestItem","LCD_NOISE_TEST","0",str,strIniFile);
    g_stCfg_FTE716_TestItem.LCD_NOISE_TEST = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();
}

void OnInit_FTE716_BasicThreshold(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////////////// FW Version
    GetPrivateProfileString( "Basic_Threshold", "FW_VER_VALUE", "0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.FW_VER_VALUE = fts_atoi(str);

    //////////////////////////////////////////////////////////// Factory ID
    GetPrivateProfileString("Basic_Threshold","Factory_ID_Number","255",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.Factory_ID_Number = fts_atoi(str);

    //////////////////////////////////////////////////////////// Project Code Test
    GetPrivateProfileString("Basic_Threshold","Project_Code"," ",str,strIniFile);
    //g_stCfg_FTE716_BasicThreshold.Project_Code.Format("%s", str);
    sprintf(g_stCfg_FTE716_BasicThreshold.Project_Code, "%s", str);

    //////////////////////////////////////////////////////////// IC Version
    GetPrivateProfileString("Basic_Threshold","IC_Version","3",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.IC_Version = fts_atoi(str);

    //////////////////////////////////////////////////////////// RawData Test
    GetPrivateProfileString("Basic_Threshold","RawDataTest_VA_Check","1",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.bRawDataTest_VA_Check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","RawDataTest_Min","5000",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.RawDataTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","RawDataTest_Max","11000",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.RawDataTest_Max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","RawDataTest_VKey_Check","1",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.bRawDataTest_VKey_Check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","RawDataTest_Min_VKey","5000",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.RawDataTest_Min_VKey = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","RawDataTest_Max_VKey","11000",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.RawDataTest_Max_VKey = fts_atoi(str);

    //////////////////////////////////////////////////////////// Channel Num Test
    GetPrivateProfileString("Basic_Threshold","ChannelNumTest_ChannelX","15",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.ChannelNumTest_ChannelXNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","ChannelNumTest_ChannelY","24",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.ChannelNumTest_ChannelYNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","ChannelNumTest_KeyNum","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.ChannelNumTest_KeyNum = fts_atoi(str);

    //////////////////////////////////////////////////////////// Reset Pin Test
    GetPrivateProfileString("Basic_Threshold","ResetPinTest_RegAddr","136",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.ResetPinTest_RegAddr = fts_atoi(str);

    //////////////////////////////////////////////////////////// Int Pin Test
    GetPrivateProfileString("Basic_Threshold","IntPinTest_RegAddr","175",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.IntPinTest_RegAddr = fts_atoi(str);

    //////////////////////////////////////////////////////////// Noise Test
    GetPrivateProfileString("Basic_Threshold","NoiseTest_Coefficient","50",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.NoiseTest_Coefficient = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_Frames","32",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.NoiseTest_Frames = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_Time","1",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.NoiseTest_Time = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_SampeMode","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.NoiseTest_SampeMode = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_NoiseMode","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.NoiseTest_NoiseMode = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold","NoiseTest_NoiseMode","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.NoiseTest_NoiseMode = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold","NoiseTest_ShowTip","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.NoiseTest_ShowTip = fts_atoi(str);

    //////////////////////////////////////////////////////////// CB Test
    GetPrivateProfileString("Basic_Threshold","CBTest_VA_Check","1",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.bCBTest_VA_Check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_Min","3",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.CbTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_Max","100",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.CbTest_Max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_VKey_Check","1",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.bCBTest_VKey_Check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_Min_Vkey","3",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.CbTest_Min_Vkey = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_Max_Vkey","100",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.CbTest_Max_Vkey = fts_atoi(str);


    //////////////////////////////////////////////////////////// Short Circuit Test
    GetPrivateProfileString("Basic_Threshold","ShortCircuit_ResMin","1200",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.ShortCircuit_ResMin = fts_atoi(str);


    ////////////////////////////////////////////////////////////open test
    GetPrivateProfileString("Basic_Threshold","OpenTest_CBMin","100",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.OpenTest_CBMin = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","OpenTest_Check_K1","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.OpenTest_Check_K1 = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","OpenTest_K1Threshold","30",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.OpenTest_K1Threshold = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","OpenTest_Check_K2","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.OpenTest_Check_K2 = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","OpenTest_K2Threshold","5",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.OpenTest_K2Threshold = fts_atoi(str);

    ////////////////////////////////////////////////////////////CB_Uniformity_test
    GetPrivateProfileString("Basic_Threshold","CBUniformityTest_Check_CHX","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.CBUniformityTest_Check_CHX = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBUniformityTest_Check_CHY","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.CBUniformityTest_Check_CHY = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBUniformityTest_Check_MinMax","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.CBUniformityTest_Check_MinMax = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBUniformityTest_CHX_Hole","20",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.CBUniformityTest_CHX_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBUniformityTest_CHY_Hole","20",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.CBUniformityTest_CHY_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBUniformityTest_MinMax_Hole","70",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.CBUniformityTest_MinMax_Hole = fts_atoi(str);

    ////////////////////////////////////////////////////////////DifferUniformityTest
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_Delta_Vol","1",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.DeltaVol = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_Check_CHX","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.DifferUniformityTest_Check_CHX = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_Check_CHY","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.DifferUniformityTest_Check_CHY = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_Check_MinMax","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.DifferUniformityTest_Check_MinMax = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_CHX_Hole","20",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.DifferUniformityTest_CHX_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_CHY_Hole","20",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.DifferUniformityTest_CHY_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_MinMax_Hole","70",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.DifferUniformityTest_MinMax_Hole = fts_atoi(str);

    ////////////////////////////////////////////////////////////Differ2UniformityTest
    GetPrivateProfileString("Basic_Threshold","Differ2UniformityTest_Check_CHX","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.Differ2UniformityTest_Check_CHX = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","Differ2UniformityTest_Check_CHY","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.Differ2UniformityTest_Check_CHY = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold","Differ2UniformityTest_CHX_Hole","20",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.Differ2UniformityTest_CHX_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","Differ2UniformityTest_CHY_Hole","20",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.Differ2UniformityTest_CHY_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","Differ2UniformityTest_Differ_Min","1000",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.Differ2UniformityTest_Differ_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","Differ2UniformityTest_Differ_Max","8000",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.Differ2UniformityTest_Differ_Max = fts_atoi(str);

    ////////////////////////////////////////////////////////////LCDNoiseTest
    GetPrivateProfileString("Basic_Threshold","LCDNoiseTest_FrameNum","200",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.LCDNoiseTest_FrameNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LCDNoiseTest_Coefficient","60",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.LCDNoiseTest_Coefficient = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LCDNoiseTest_NoiseMode","0",str,strIniFile);
    g_stCfg_FTE716_BasicThreshold.LCDNoiseTest_NoiseMode = fts_atoi(str);


    FTS_TEST_FUNC_EXIT();

}
void SetTestItem_FTE716(void)
{
    g_TestItemNum = 0;

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////FACTORY_ID_TEST
    if ( g_stCfg_FTE716_TestItem.FACTORY_ID_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_FACTORY_ID_TEST);
    }

    //////////////////////////////////////////////////Project Code Test
    if ( g_stCfg_FTE716_TestItem.PROJECT_CODE_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_PROJECT_CODE_TEST);
    }

    //////////////////////////////////////////////////FW Version Test
    if ( g_stCfg_FTE716_TestItem.FW_VERSION_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_FW_VERSION_TEST);
    }

    //////////////////////////////////////////////////IC Version Test
    if ( g_stCfg_FTE716_TestItem.IC_VERSION_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_IC_VERSION_TEST);
    }

    //////////////////////////////////////////////////Enter Factory Mode
    fts_SetTestItemCodeName(Code_FTE716_ENTER_FACTORY_MODE);

    //////////////////////////////////////////////////CHANNEL_NUM_TEST
    if ( g_stCfg_FTE716_TestItem.CHANNEL_NUM_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_CHANNEL_NUM_TEST);
    }

    //////////////////////////////////////////////////CB_TEST
    if ( g_stCfg_FTE716_TestItem.CB_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_CB_TEST);
    }

    //////////////////////////////////////////////////CB_UNIFORMITY_TEST
    if ( g_stCfg_FTE716_TestItem.CB_UNIFORMITY_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_CB_UNIFORMITY_TEST);
    }



    //////////////////////////////////////////////////NOISE_TEST
    if ( g_stCfg_FTE716_TestItem.NOISE_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_NOISE_TEST);
    }

    //////////////////////////////////////////////////LCD_NOISE_TEST
    if ( g_stCfg_FTE716_TestItem.LCD_NOISE_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_LCD_NOISE_TEST);
    }

    //////////////////////////////////////////////////RawData Test
    if ( g_stCfg_FTE716_TestItem.RAWDATA_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_RAWDATA_TEST);
    }


    //////////////////////////////////////////////////DIFFER_UNIFORMITY_TEST
    if ( g_stCfg_FTE716_TestItem.DIFFER_UNIFORMITY_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_DIFFER_UNIFORMITY_TEST);
    }


    //////////////////////////////////////////////////OPEN_TEST
    if ( g_stCfg_FTE716_TestItem.OPEN_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_OPEN_TEST);
    }

    //////////////////////////////////////////////////SHORT_CIRCUIT_TEST
    if ( g_stCfg_FTE716_TestItem.SHORT_CIRCUIT_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_SHORT_CIRCUIT_TEST) ;
    }

    //////////////////////////////////////////////////DIFFER2_UNIFORMITY_TEST
    if ( g_stCfg_FTE716_TestItem.DIFFER2_UNIFORMITY_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FTE716_DIFFER2_UNIFORMITY_TEST);
    }

    FTS_TEST_FUNC_EXIT();

}

#endif

