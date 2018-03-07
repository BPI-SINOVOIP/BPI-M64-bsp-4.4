# SUNXI Platform Support
snd-soc-sunxi-objs := sunxi-pcm.o
snd-soc-sunxi-hdmi-objs := sunxi-hdmi.o
ifneq ($(CONFIG_ARCH_SUN8IW6),)
snd-soc-sunxi-daudio-objs := sunxi-i2s.o
else
snd-soc-sunxi-daudio-objs := sunxi-daudio.o
endif
snd-soc-sunxi-spdif-objs := sunxi-spdif.o spdif-utils.o
snd-soc-sunxi-cpudai-objs := sunxi-cpudai.o
snd-soc-sunxi-dsd-objs := sunxi-dsd.o
snd-soc-sunxi-dmic-objs := sunxi-dmic.o
snd-soc-sunxi-rwfunc-objs := sunxi_rw_func.o
snd-soc-sunxi-inter-i2s-objs := sunxi-inter-i2s.o
snd-soc-sunxi-bb-dai-objs := bb_dai.o
snd-soc-sun8iw6-soc-tdm-objs := sun8iw6-tdm.o

obj-$(CONFIG_SND_SUNXI_SOC) += snd-soc-sunxi.o
obj-$(CONFIG_SND_SUNXI_SOC_HDMI) += snd-soc-sunxi-hdmi.o
obj-$(CONFIG_SND_SUNXI_SOC_DAUDIO) += snd-soc-sunxi-daudio.o
obj-$(CONFIG_SND_SUNXI_SOC_SPDIF) += snd-soc-sunxi-spdif.o
obj-$(CONFIG_SND_SUNXI_SOC_CPUDAI) += snd-soc-sunxi-cpudai.o
obj-$(CONFIG_SND_SUNXI_SOC_DSD) += snd-soc-sunxi-dsd.o
obj-$(CONFIG_SND_SUNXI_SOC_DMIC) += snd-soc-sunxi-dmic.o
obj-$(CONFIG_SND_SUNXI_SOC_RWFUNC) += snd-soc-sunxi-rwfunc.o
obj-$(CONFIG_SND_SUNXI_SOC_INTER_I2S) += snd-soc-sunxi-inter-i2s.o
obj-$(CONFIG_SND_SUN8IW6_VIRBB_DAI) += snd-soc-sunxi-bb-dai.o
obj-$(CONFIG_SND_SUN8IW6_SOC_TDM) += snd-soc-sun8iw6-soc-tdm.o

# SUNXI Codec Embedded Support
obj-$(CONFIG_SND_SUN8IW11_CODEC) += sun8iw11-codec.o
obj-$(CONFIG_SND_SUN8IW12_CODEC) += sun8iw12-codec.o
obj-$(CONFIG_SND_SUN50IW1_CODEC) += sun50iw1-codec.o
obj-$(CONFIG_SND_SUN50IW3_CODEC) += sun50iw3-codec.o

# SUNXI Machine Support
obj-$(CONFIG_SND_SUNXI_SOC_SUN8IW11_CODEC) += sun8iw11-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN8IW12_CODEC) += sun8iw12-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW1_CODEC) += sun50iw1-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW3_CODEC) += sun50iw3-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_HDMIAUDIO) += sunxi-sndhdmi.o
ifneq ($(CONFIG_ARCH_SUN8IW6),)
obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_DAUDIO) += sunxi-sndi2s.o
else
obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_DAUDIO) += sunxi-snddaudio.o
endif
obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_SPDIF) += sunxi-sndspdif.o
obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_DSD) += sunxi-snddsd.o
obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_DMIC) += sunxi-snddmic.o
# exernal codec for sun8iw6
obj-$(CONFIG_SND_SUN8IW6_DAUDIO0_MACHINE) += sun8iw6-snddaudio0.o

