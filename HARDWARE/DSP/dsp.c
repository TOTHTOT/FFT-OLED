/*
 * @Description: dsp.c
 * @Author: TOTHTOT
 * @Date: 2022-05-22 16:14:57
 * @LastEditTime: 2022-05-22 19:15:48
 * @LastEditors: TOTHTOT
 * @FilePath: \USERe:\Learn\stm32\实例\FFT+OLED\HARDWARE\DSP\dsp.c
 */
#include "dsp.h"
#include "math.h"
u32 lBufInArray[NPT];      //输入数据
u32 lBufOutArray[NPT / 2]; //输出信息 NPT个频率为:0-Fs/NPT-MAX
u32 lBufMagArray[NPT / 2]; //幅值分量 NPT个频率对应的分量

/******************************************************************
函数名称:InitBufInArray()
函数功能:模拟采样数据，采样数据中包含3种频率正弦波(350Hz，8400Hz，18725Hz)
参数说明:
备    注:在lBufInArray数组中，每个数据的高16位存储采样数据的实部低16位存储采样数据的虚部(总是为0)
*******************************************************************/
void InitBufInArray(void)
{
    u16 i;
    float fx;
    for (i = 0; i < NPT; i++)
    {
        fx = 500 +                                      // 0HZ     交流分量0    直流分量 500
             1500 * sin(PI2 * i * 350.0 / Fs) + 1500 +  // 350HZ   交流分量1500 直流分量1500
             2700 * sin(PI2 * i * 8400.0 / Fs) + 2700 + // 8400HZ  交流分量2700 直流分量2700
             4000 * sin(PI2 * i * 18725.0 / Fs) + 4000+ // 18725HZ 交流分量4000 直流分量4000
             2000 * sin(PI2 * i * 26250.0 / Fs) + 2000; // 10000HZ 交流分量2000 直流分量2000
        //数据特征：交流分量350HZ 1500    8400HZ 2700  18725HZ  4000  直流分量500+1500+2700+4000=8700
        //为了模拟adc都是正数值 交流也加上直流分量变为正数
        lBufInArray[i] = ((u16)fx) << 16;
    }
}

/******************************************************************
函数名称:GetPowerMag()
函数功能:计算各次谐波幅值
参数说明:
备　　注:先将lBufOutArray分解成实部(X)和虚部(Y)，然后计算幅值(sqrt(X*X+Y*Y)
*******************************************************************/
void GetPowerMag(void)
{
    s16 lX, lY;
    float X, Y, Mag;
    u16 i;
    for (i = 0; i < NPT / 2; i++)
    {
        lX = (lBufOutArray[i] << 16) >> 16;
        lY = (lBufOutArray[i] >> 16);
        X = NPT * ((float)lX) / 32768;
        Y = NPT * ((float)lY) / 32768;
        Mag = sqrt(X * X + Y * Y) / NPT;
        if (i == 0)
            lBufMagArray[i] = (unsigned long)(Mag * 32768);
        else
            lBufMagArray[i] = (unsigned long)(Mag * 65536);
    }
}
