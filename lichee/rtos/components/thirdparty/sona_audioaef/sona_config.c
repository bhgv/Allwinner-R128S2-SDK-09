/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
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

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "sona_config.h"

SONA_CONFIG_DECLARE(sona_audioaef_example);

sona_config_t sona_global_configs[] = {
	SONA_CONFIG_LABEL(sona_audioaef_example),
};

void sona_config_global_configs_print(void)
{
	size_t i;
	for (i = 0; i < sizeof(sona_global_configs) / sizeof(sona_global_configs[0]); ++i)
		printf("%s\n", sona_global_configs[i].name);
}

sona_config_t *sona_config_find(const char *name)
{
	size_t i;

	if (!name) {
		printf("invalid config name\n");
		return NULL;
	}

	for (i = 0; i < sizeof(sona_global_configs) / sizeof(sona_global_configs[0]); ++i) {
		if (0 == strcmp(sona_global_configs[i].name, name))
			return &sona_global_configs[i];
	}
	return NULL;
}