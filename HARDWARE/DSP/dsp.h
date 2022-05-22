/*
 * @Description: dsp.h
 * @Author: TOTHTOT
 * @Date: 2022-05-22 16:15:18
 * @LastEditTime: 2022-05-22 16:37:08
 * @LastEditors: TOTHTOT
 * @FilePath: \USERe:\Learn\stm32\实例\FFT+OLED\HARDWARE\DSP\dsp.h
 */
#ifndef __DSP_H
#define __DSP_H

#include "sys.h"
#include "stm32_dsp.h"

#define PI2 6.28318530717959 
#define NPT 256                //采样点数64 256 1024
#define Fs  44800              //采样频率44800 要采集的是音频信号，音频信号的频率范围是20Hz到20KHz，所以我使用的采用频率是44800Hz。那么在进行256点FFT时，将得到44800Hz / 256 = 175Hz的频率分辨率
                               //FFT的到的频率都是整数，想保留n位小数，Fs*10^n
                               //例如这个例子实际采样频率是44800,结果想保留1位小数，FS改成448000,结果除以10就是实际频率
extern u32 lBufInArray[NPT];     
extern u32 lBufOutArray[NPT / 2];
extern u32 lBufMagArray[NPT / 2];
void InitBufInArray(void);
void GetPowerMag(void);

#endif /* __DSP_H */    
