## Some AT command test

Refer to the T-A7670X instruction manual for more [A76XX Series_Audio_Application Note](../../../hardware/A76XX_series/A76XX_Series_AT_Command_Manual_V1.09.pdf) 

| command      | explain                              | Response     |
| ------------ | ------------------------------------ | ------------ |
| AT           | AT test instruction                  | OK           |
| ATI          | version information                  |              |
| AT+CPIN?     | Example Query the SIM card status    | +CPIN: READY |
| AT+CGREG?    | Network Registration Status          | +CGREG: 0,1  |
| AT+CSQ       | Network signal quality query         |              |
| AT+COPS?     | Querying current Carrier Information |              |


## Audio test

1. TTS(Text-to-Speech) test

| `AT+CTTSPARAM=1,3,0,1,1,0`  | TTS parameters setting    |
| :-------------------------- | :------------------------ |
| `AT+CTTS=2,"1234567890ABC"` | synth and play ASCII text |

2. Play an audio file

| `AT+CCMXPLAY="c:/iPhone_Ring.mp3",0,255` |              |
| :--------------------------------------- | :----------- |
| `AT+CCMXSTOP`                            | Stop playing |

## Call test

| `AT+CSQ`            | Query signal quality, GSM antenna required            |
| :------------------ | :---------------------------------------------------- |
| `ATDxxxxxxxxx;`     | Call to dial a number，`xxxxxxxxx` is the phone number |
| `ATA`               | Call answer                                           |
| `AT+CHUP`           | Hang up call                                          |
| `AT+COUTGAIN=(0-7)` | Adjust out gain, 4 is the default value               |
| `AT+CMICGAIN=(0-7)` | Adjust mic gain, 4 is the default value               |
| `AT+VMUTE=0/1`      | Speaker mute control; 0: mute off; 1: mute on         |
| `AT+CMUT=0/1`       | Microphone mute control; 0: mute off; 1: mute on      |

## SMS test
Select SMS message format; 0:PUD mode; 1:TEXT mode

`AT+CMGF=1`

Send message
~~~
AT+CMGS="Telephone number"       ; 先发送这个命令
Text is entered                  ; 然后输入内容，AT+CMGF=1 文本模式
<CTRL-Z/ESC>                     ; 按下ctrl+z键发送
~~~

Read message

`AT+CMGR=1`

## Other information

[A76XX Series_HTTP(S)_Application](../../../hardware/A76XX_series/A76XX%20Series_HTTP(S)_Application%20Note_V1.03.pdf)

[A76XX Series MQTT EX_Application](../../../hardware/A76XX_series/A76XX%20Series%20MQTT%20EX_Application%20Note_V1.00.pdf)

[A76XX Series_Audio_Application](../../../hardware/A76XX_series/A76XX%20Series_Audio_Application%20Note_V1.03.pdf)






