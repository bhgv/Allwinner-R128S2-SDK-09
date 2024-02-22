/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define __NAND_COMMON_INFO_H__

#ifndef _NAND_INFO_H
#define _NAND_INFO_H

#define  NAND_VERSION_0                 0x03
#define  NAND_VERSION_1                 0x01

//define the return value

#define NAND_OP_TRUE            (0)                     //define the successful return value
#define NAND_OP_FALSE           (-1)                    //define the failed return value

#define ECC_LIMIT               10                   //reach the limit of the ability of ECC
#define ERR_ECC                 (-2)                  //too much ecc error
#define ERR_TIMEOUT             (-3)                      //hardware timeout
#define ERR_PARA                (-4)

#define ERR_INVALIDPHYADDR		  5

#define ERR_NO_10                (-10)
#define ERR_NO_11                (-11)
#define ERR_NO_12                (-12)
#define ERR_NO_13                (-13)
#define ERR_NO_14                (-14)
#define ERR_NO_15                (-15)
#define ERR_NO_16                (-16)
#define ERR_NO_17                (-17)
#define ERR_NO_18                (-18)
#define ERR_NO_19                (-19)
#define ERR_NO_20                (-20)
#define ERR_NO_21                (-21)
#define ERR_NO_22                (-22)
#define ERR_NO_23                (-23)
#define ERR_NO_24                (-24)
#define ERR_NO_25                (-25)
#define ERR_NO_26                (-26)
#define ERR_NO_27                (-27)
#define ERR_NO_28                (-28)
#define ERR_NO_29                (-29)
#define ERR_NO_30                (-30)
#define ERR_NO_31                (-31)
#define ERR_NO_32                (-32)
#define ERR_NO_33                (-33)
#define ERR_NO_34                (-34)
#define ERR_NO_35                (-35)
#define ERR_NO_36                (-36)
#define ERR_NO_37                (-37)
#define ERR_NO_38                (-38)
#define ERR_NO_39                (-39)
#define ERR_NO_40                (-40)
#define ERR_NO_41                (-41)
#define ERR_NO_42                (-42)
#define ERR_NO_43                (-43)
#define ERR_NO_44                (-44)
#define ERR_NO_45                (-45)
#define ERR_NO_46                (-46)
#define ERR_NO_47                (-47)
#define ERR_NO_48                (-48)
#define ERR_NO_49                (-49)
#define ERR_NO_50                (-50)
#define ERR_NO_51                (-51)
#define ERR_NO_52                (-52)
#define ERR_NO_53                (-53)
#define ERR_NO_54                (-54)
#define ERR_NO_55                (-55)
#define ERR_NO_56                (-56)
#define ERR_NO_57                (-57)
#define ERR_NO_58                (-58)
#define ERR_NO_59                (-59)
#define ERR_NO_60                (-60)
#define ERR_NO_61                (-61)
#define ERR_NO_62                (-62)
#define ERR_NO_63                (-63)
#define ERR_NO_64                (-64)
#define ERR_NO_65                (-65)
#define ERR_NO_66                (-66)
#define ERR_NO_67                (-67)
#define ERR_NO_68                (-68)
#define ERR_NO_69                (-69)
#define ERR_NO_70                (-70)
#define ERR_NO_71                (-71)
#define ERR_NO_72                (-72)
#define ERR_NO_73                (-73)
#define ERR_NO_74                (-74)
#define ERR_NO_75                (-75)
#define ERR_NO_76                (-76)
#define ERR_NO_77                (-77)
#define ERR_NO_78                (-78)
#define ERR_NO_79                (-79)
#define ERR_NO_80                (-80)
#define ERR_NO_81                (-81)
#define ERR_NO_82                (-82)
#define ERR_NO_83                (-83)
#define ERR_NO_84                (-84)
#define ERR_NO_85                (-85)
#define ERR_NO_86                (-86)
#define ERR_NO_87                (-87)
#define ERR_NO_88                (-88)
#define ERR_NO_89                (-89)
#define ERR_NO_90                (-90)
#define ERR_NO_91                (-91)
#define ERR_NO_92                (-92)
#define ERR_NO_93                (-93)
#define ERR_NO_94                (-94)
#define ERR_NO_95                (-95)
#define ERR_NO_96                (-96)
#define ERR_NO_97                (-97)
#define ERR_NO_98                (-98)
#define ERR_NO_99                (-99)
#define ERR_NO_100                (-100)
#define ERR_NO_101                (-101)
#define ERR_NO_102                (-102)
#define ERR_NO_103                (-103)
#define ERR_NO_104                (-104)
#define ERR_NO_105                (-105)
#define ERR_NO_106                (-106)
#define ERR_NO_107                (-107)
#define ERR_NO_108                (-108)
#define ERR_NO_109                (-109)
#define ERR_NO_110                (-110)
#define ERR_NO_111                (-111)
#define ERR_NO_112                (-112)
#define ERR_NO_113                (-113)
#define ERR_NO_114                (-114)
#define ERR_NO_115                (-115)
#define ERR_NO_116                (-116)
#define ERR_NO_117                (-117)
#define ERR_NO_118                (-118)
#define ERR_NO_119                (-119)
#define ERR_NO_120                (-120)
#define ERR_NO_121                (-121)
#define ERR_NO_122                (-122)
#define ERR_NO_123                (-123)
#define ERR_NO_124                (-124)
#define ERR_NO_125                (-125)
#define ERR_NO_126                (-126)
#define ERR_NO_127                (-127)
#define ERR_NO_128                (-128)
#define ERR_NO_129                (-129)
#define ERR_NO_130                (-130)
#define ERR_NO_131                (-131)
#define ERR_NO_132                (-132)
#define ERR_NO_133                (-133)



#endif /*__NAND_INFO_H__*/
