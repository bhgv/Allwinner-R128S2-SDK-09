
#ifndef _PM_WAKERES_H_
#define _PM_WAKERES_H_

typedef enum {
	PM_RES_POWER_RTC_LDO = 0,
	PM_RES_POWER_EXT_LDO,
	PM_RES_POWER_ALDO,
	PM_RES_POWER_DCDC,
	PM_RES_POWER_TOP_LDO,
	PM_RES_POWER_AON_LDO, //5
	PM_RES_POWER_APP_LDO,
	PM_RES_POWER_RV_LDO,
	PM_RES_POWER_DSP_LDO,
	PM_RES_POWER_SW,
	PM_RES_POWER_SW_VDDIO_SIP, //10
	PM_RES_POWER_SW_APP,
	PM_RES_POWER_SW_RF,
	PM_RES_POWER_SW_WF,
	PM_RES_POWER_SWM,
	PM_RES_POWER_MAX,

	PM_RES_CLOCK_OSC32K = 0,
	PM_RES_CLOCK_RC32K,
	PM_RES_CLOCK_HOSC,
	PM_RES_CLOCK_DPLL1,
	PM_RES_CLOCK_DPLL2,
	PM_RES_CLOCK_DPLL3, //5
	PM_RES_CLOCK_AUDPLL,
	PM_RES_CLOCK_DPLL_AUDPLL_LDO,
	PM_RES_CLOCK_RCOSC_AUD_RC_HF,
	PM_RES_CLOCK_APB_NO_NEED_TO_32K,
	PM_RES_CLOCK_AHB_NO_NEED_TO_32K, //10
	PM_RES_CLOCK_APB_FROM_RCO_HF,
	PM_RES_CLOCK_AHB_FROM_RCO_HF,
	PM_RES_CLOCK_RCO_CALIB,
	PM_RES_CLOCK_MAX,

	PM_RES_ANALOG_CODEC_ADC = 0,
	PM_RES_ANALOG_CODEC_DAC,
	PM_RES_ANALOG_GPADC,
	PM_RES_ANALOG_EFUSE,
	PM_RES_ANALOG_USB_PHY,
	PM_RES_ANALOG_PSRAM_PHY, //5
	PM_RES_ANALOG_RTC_GPIO,
	PM_RES_ANALOG_AON_GPIO,
	PM_RES_ANALOG_RFIP0,
	PM_RES_ANALOG_RFIP1,
	PM_RES_ANALOG_MAX, //10
} pm_resource_type_t;


#define PM_RES_BIT(_t) (0x1 << (_t))
#define PM_RES_STANDBY_NULL           { \
		.pwrcfg = 0x0, \
		.clkcfg = 0x0, \
		.anacfg = 0x0, \
		}

#define PM_RES_HIBERNATION     { \
		.pwrcfg = (PM_RES_BIT(PM_RES_POWER_RTC_LDO) \
			| PM_RES_BIT(PM_RES_POWER_EXT_LDO) \
			),\
		.clkcfg = (PM_RES_BIT(PM_RES_CLOCK_OSC32K)  \
			| PM_RES_BIT(PM_RES_CLOCK_RC32K) \
			),\
		.anacfg = (PM_RES_BIT(PM_RES_ANALOG_RTC_GPIO) \
			),\
		}

#define PM_RES_STANDBY_RTC     { \
		.pwrcfg = (PM_RES_BIT(PM_RES_POWER_RTC_LDO) \
			| PM_RES_BIT(PM_RES_POWER_EXT_LDO) \
			),\
		.clkcfg = (PM_RES_BIT(PM_RES_CLOCK_OSC32K)  \
			| PM_RES_BIT(PM_RES_CLOCK_RC32K) \
			),\
		.anacfg = (PM_RES_BIT(PM_RES_ANALOG_RTC_GPIO) \
			),\
		}

#define PM_RES_STANDBY_WUP   { \
		.pwrcfg = (PM_RES_BIT(PM_RES_POWER_RTC_LDO) \
			| PM_RES_BIT(PM_RES_POWER_EXT_LDO) \
			),\
		.clkcfg = (PM_RES_BIT(PM_RES_CLOCK_OSC32K)  \
			| PM_RES_BIT(PM_RES_CLOCK_RC32K) \
			),\
		.anacfg = (PM_RES_BIT(PM_RES_ANALOG_RTC_GPIO) \
			),\
		}

#define PM_RES_STANDBY_WUPTMR  { \
		.pwrcfg = (PM_RES_BIT(PM_RES_POWER_RTC_LDO) \
			| PM_RES_BIT(PM_RES_POWER_EXT_LDO) \
			| PM_RES_BIT(PM_RES_POWER_AON_LDO) \
			| PM_RES_BIT(PM_RES_POWER_SWM) \
			),\
		.clkcfg = (PM_RES_BIT(PM_RES_CLOCK_OSC32K)  \
			| PM_RES_BIT(PM_RES_CLOCK_RC32K) \
			),\
		.anacfg = (PM_RES_BIT(PM_RES_ANALOG_RTC_GPIO) \
			| PM_RES_BIT(PM_RES_ANALOG_AON_GPIO) \
			),\
		}

#define PM_RES_STANDBY_WLAN    { \
		.pwrcfg = (PM_RES_BIT(PM_RES_POWER_RTC_LDO) \
			| PM_RES_BIT(PM_RES_POWER_EXT_LDO) \
			| PM_RES_BIT(PM_RES_POWER_AON_LDO) \
			| PM_RES_BIT(PM_RES_POWER_SWM) \
			),\
		.clkcfg = (PM_RES_BIT(PM_RES_CLOCK_OSC32K)  \
			| PM_RES_BIT(PM_RES_CLOCK_RC32K) \
			| PM_RES_BIT(PM_RES_CLOCK_DPLL1) \
			| PM_RES_BIT(PM_RES_CLOCK_DPLL2) \
			),\
		.anacfg = (PM_RES_BIT(PM_RES_ANALOG_RTC_GPIO) \
			| PM_RES_BIT(PM_RES_ANALOG_AON_GPIO) \
			),\
		}

#define PM_RES_STANDBY_BLE    { \
		.pwrcfg = (PM_RES_BIT(PM_RES_POWER_RTC_LDO) \
			| PM_RES_BIT(PM_RES_POWER_EXT_LDO) \
			| PM_RES_BIT(PM_RES_POWER_AON_LDO) \
			| PM_RES_BIT(PM_RES_POWER_SWM) \
			),\
		.clkcfg = (PM_RES_BIT(PM_RES_CLOCK_OSC32K)  \
			| PM_RES_BIT(PM_RES_CLOCK_RC32K) \
			| PM_RES_BIT(PM_RES_CLOCK_DPLL1) \
			| PM_RES_BIT(PM_RES_CLOCK_DPLL2) \
			| PM_RES_BIT(PM_RES_CLOCK_RCO_CALIB) \
			),\
		.anacfg = (PM_RES_BIT(PM_RES_ANALOG_RTC_GPIO) \
			| PM_RES_BIT(PM_RES_ANALOG_AON_GPIO) \
			),\
		}

#define PM_RES_STANDBY_BT    PM_RES_STANDBY_BLE

#define PM_RES_STANDBY_MAD     { \
		.pwrcfg = (PM_RES_BIT(PM_RES_POWER_RTC_LDO) \
			| PM_RES_BIT(PM_RES_POWER_EXT_LDO) \
			| PM_RES_BIT(PM_RES_POWER_ALDO) \
			| PM_RES_BIT(PM_RES_POWER_AON_LDO) \
			| PM_RES_BIT(PM_RES_POWER_SWM) \
			),\
		.clkcfg = (PM_RES_BIT(PM_RES_CLOCK_OSC32K)  \
			| PM_RES_BIT(PM_RES_CLOCK_RC32K) \
			| PM_RES_BIT(PM_RES_CLOCK_RCOSC_AUD_RC_HF) \
			| PM_RES_BIT(PM_RES_CLOCK_APB_FROM_RCO_HF) \
			| PM_RES_BIT(PM_RES_CLOCK_AHB_FROM_RCO_HF) \
			),\
		.anacfg = (PM_RES_BIT(PM_RES_ANALOG_CODEC_ADC) \
			| PM_RES_BIT(PM_RES_ANALOG_RTC_GPIO) \
			| PM_RES_BIT(PM_RES_ANALOG_AON_GPIO) \
			),\
		}

#define PM_RES_STANDBY_GPADC   { \
		.pwrcfg = (PM_RES_BIT(PM_RES_POWER_RTC_LDO) \
			| PM_RES_BIT(PM_RES_POWER_EXT_LDO) \
			| PM_RES_BIT(PM_RES_POWER_AON_LDO) \
			| PM_RES_BIT(PM_RES_POWER_SWM) \
			),\
		.clkcfg = (PM_RES_BIT(PM_RES_CLOCK_OSC32K)  \
			| PM_RES_BIT(PM_RES_CLOCK_RC32K) \
			| PM_RES_BIT(PM_RES_CLOCK_APB_FROM_RCO_HF) \
			),\
		.anacfg = (PM_RES_BIT(PM_RES_ANALOG_GPADC) \
			| PM_RES_BIT(PM_RES_ANALOG_RTC_GPIO) \
			| PM_RES_BIT(PM_RES_ANALOG_AON_GPIO) \
			),\
		}

#define PM_RES_STANDBY_LPUART  { \
		.pwrcfg = (PM_RES_BIT(PM_RES_POWER_RTC_LDO) \
			| PM_RES_BIT(PM_RES_POWER_EXT_LDO) \
			| PM_RES_BIT(PM_RES_POWER_AON_LDO) \
			| PM_RES_BIT(PM_RES_POWER_SWM) \
			),\
		.clkcfg = (PM_RES_BIT(PM_RES_CLOCK_OSC32K)  \
			| PM_RES_BIT(PM_RES_CLOCK_RC32K) \
			| PM_RES_BIT(PM_RES_CLOCK_APB_FROM_RCO_HF) \
			),\
		.anacfg = (PM_RES_BIT(PM_RES_ANALOG_RTC_GPIO) \
			| PM_RES_BIT(PM_RES_ANALOG_AON_GPIO) \
			),\
		}

#define PM_RES_STANDBY_GPIO    { \
		.pwrcfg = (PM_RES_BIT(PM_RES_POWER_RTC_LDO) \
			| PM_RES_BIT(PM_RES_POWER_EXT_LDO) \
			| PM_RES_BIT(PM_RES_POWER_AON_LDO) \
			| PM_RES_BIT(PM_RES_POWER_SWM) \
			),\
		.clkcfg = (PM_RES_BIT(PM_RES_CLOCK_OSC32K)  \
			| PM_RES_BIT(PM_RES_CLOCK_RC32K) \
			| PM_RES_BIT(PM_RES_CLOCK_APB_FROM_RCO_HF) \
			),\
		.anacfg = (PM_RES_BIT(PM_RES_ANALOG_RTC_GPIO) \
			| PM_RES_BIT(PM_RES_ANALOG_AON_GPIO) \
			),\
		}

#define PM_RES_STANDBY_SLEEP PM_RES_STANDBY_GPIO


struct pm_wakeres {
	const uint32_t  pwrcfg;
	const uint32_t  clkcfg;
	const uint32_t  anacfg;
};


int pm_wakeres_update(void);
uint32_t pm_wakeres_get_pwrcfg(void);
uint32_t pm_wakeres_get_clkcfg(void);
uint32_t pm_wakeres_get_anacfg(void);


#endif
