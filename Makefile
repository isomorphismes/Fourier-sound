# Recipes are direct build-tool invocations using make's fixed shell interface.
# No app Java/Kotlin, Gradle, or maintained shell program.
BUILD ?= build
CC ?= cc
CFLAGS ?= -O2 -g
WARN = -Wall -Wextra -Werror -Wpedantic -Wshadow
INCLUDES = -Iaudio/interface -Iaudio/android -Ifourier -Imath -Irender -Irender/android -Iacceptance
COMMON = audio/interface/pcm_ring.c audio/interface/audio_result.c fourier/pcm_block.c acceptance/microphone_check.c
HEADERS = $(wildcard audio/interface/*.h audio/android/*.h fourier/*.h math/*.h render/*.h render/android/*.h acceptance/*.h)
RENDER = fourier/dft.c fourier/fft.c fourier/framing.c fourier/complex_field.c fourier/sparse_series.c render/wegert.c render/complex_plot.c render/ppm.c
ANDROID_HOME ?= /opt/android-sdk
NDK ?= $(ANDROID_HOME)/ndk/27.2.12479018
TOOLCHAIN = $(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin
GLUE = $(NDK)/sources/android/native_app_glue
TOOLS = $(ANDROID_HOME)/build-tools/36.0.0
ANDROID_JAR = $(ANDROID_HOME)/platforms/android-36/android.jar
ANDROID_KEYSTORE ?= .test-signing/_/build/app/wegert-debug.keystore
VERSION_CODE ?= 1
# Materialized at the exact revision pinned in ci/platforms.tsv.
ANDROID_NDK_REPO ?= .android-ndk
APK_PACKAGER = $(ANDROID_NDK_REPO)/apk/build-nativeactivity-apk.sh

define package_native
	env ANDROID_HOME="$(ANDROID_HOME)" ANDROID_BUILD_TOOLS="$(TOOLS)" \
	ANDROID_PACKAGE_ID="$(1)" ANDROID_VERSION_CODE="$(2)" ANDROID_VERSION_NAME="$(3)" \
	ANDROID_MIN_SDK=26 ANDROID_TARGET_SDK=36 ANDROID_KEYSTORE="$(abspath $(ANDROID_KEYSTORE))" \
	ANDROID_KEYSTORE_TYPE=JKS ANDROID_KEY_ALIAS=wegert-debug \
	ANDROID_STORE_PASSWORD=wegert-debug ANDROID_KEY_PASSWORD=wegert-debug \
	ANDROID_EXPECTED_CERT_SHA256=de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9 \
	bash "$(APK_PACKAGER)" "$(4)" "$(5)" "$(6)" "$(7)"
	sha256sum $(7) > $(7).sha256
endef

.PHONY: all test android android-ick-leaf-armv7 android-miro-release android-ick-leaf-miro-release miro-release-compare
all: test

$(BUILD)/pcm-test: tests/pcm_test.c $(COMMON) $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) $< $(COMMON) -pthread -lm -o $@

$(BUILD)/backend-test: tests/backend_test.c tests/fake/aaudio/AAudio.h audio/android/aaudio_input.c $(COMMON) $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) -Itests/fake tests/backend_test.c audio/android/aaudio_input.c $(COMMON) -pthread -lm -o $@

$(BUILD)/output-backend-test: tests/output_backend_test.c tests/fake/aaudio/AAudio.h audio/android/aaudio_output.c audio/interface/audio_result.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) -Itests/fake tests/output_backend_test.c audio/android/aaudio_output.c audio/interface/audio_result.c -lm -o $@

$(BUILD)/speaker-input-test: tests/speaker_input_test.c audio/interface/speaker_input.c audio/interface/speaker_input.h audio/interface/audio_input.h
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) tests/speaker_input_test.c audio/interface/speaker_input.c -lm -o $@

$(BUILD)/render-test: tests/render_test.c fourier/pcm_block.c $(RENDER) $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) tests/render_test.c fourier/pcm_block.c $(RENDER) -lm -o $@

$(BUILD)/fft-test: tests/fft_test.c fourier/dft.c fourier/fft.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) tests/fft_test.c fourier/dft.c fourier/fft.c -lm -o $@

$(BUILD)/sparse-series-test: tests/sparse_series_test.c fourier/sparse_series.c fourier/complex_field.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) $< fourier/sparse_series.c fourier/complex_field.c -lm -o $@

MEDIA_DIR ?= $(BUILD)/media
SOUNDS_DIR ?= .sounds
SOUNDS_REV = cde1a53a099d94881145498b4b906da81eba1cc5
MEDIA_FIXTURES = $(addprefix $(MEDIA_DIR)/,thunder-rain.s16 iceberg-contact.s16 dvorak-largo.s16 bartok-sonatina.s16 russolo-corale.s16 russolo-serenata.s16)

.PHONY: media-corpus-check media-fetch
media-corpus-check:
	test "$$(git -C '$(SOUNDS_DIR)' rev-parse HEAD)" = '$(SOUNDS_REV)'

define media_fixture
$(MEDIA_DIR)/$(1).s16: $(SOUNDS_DIR)/audio/original/$(2) | media-corpus-check
	mkdir -p $$(@D)
	ffmpeg -nostdin -loglevel error -y -ss $(3) -i $$< -t 8 -ac 1 -ar 44100 -f s16le $$@
endef
$(eval $(call media_fixture,thunder-rain,thunder_and_rain_on_a_veranda.ogg,0))
$(eval $(call media_fixture,iceberg-contact,noaa_iceberg_harmonic_tremor.wav,0))
$(eval $(call media_fixture,dvorak-largo,dvorak_new_world_ii_largo.ogg,60))
$(eval $(call media_fixture,bartok-sonatina,bartok_sonatina.ogg,10))
$(eval $(call media_fixture,russolo-corale,russolo_corale_1921.mp3,0))
$(eval $(call media_fixture,russolo-serenata,russolo_serenata_1921.mp3,0))
media-fetch: $(MEDIA_FIXTURES)

$(BUILD)/media-fixture-test: tests/media_fixture_test.c $(RENDER) fourier/pcm_block.c audio/interface/speaker_input.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) $< $(RENDER) fourier/pcm_block.c audio/interface/speaker_input.c -lm -o $@

.PHONY: media-test
media-test: $(BUILD)/media-fixture-test $(MEDIA_FIXTURES)
	$(BUILD)/media-fixture-test $(MEDIA_FIXTURES)

test: $(BUILD)/sparse-series-test $(BUILD)/media-fixture-test

$(BUILD)/framing-test: tests/framing_test.c fourier/framing.c fourier/fft.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) tests/framing_test.c fourier/framing.c fourier/fft.c -lm -o $@

$(BUILD)/voice-signal-test: tests/voice_signal_test.c fourier/fft.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) tests/voice_signal_test.c fourier/fft.c -lm -o $@

$(BUILD)/ick-polynomial-leaf-test: tests/ick_polynomial_leaf_test.c fourier/ick_polynomial_leaf.c fourier/complex_field.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) tests/ick_polynomial_leaf_test.c fourier/ick_polynomial_leaf.c fourier/complex_field.c -lm -o $@

$(BUILD)/rgb24-rgba8888-test: tests/rgb24_rgba8888_test.c render/rgb24_rgba8888.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) tests/rgb24_rgba8888_test.c render/rgb24_rgba8888.c -o $@

$(BUILD)/native-window-output-test: tests/native_window_output_test.c tests/fake/android/native_window.h render/android/native_window_output.c render/rgb24_rgba8888.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) -Itests/fake $(INCLUDES) tests/native_window_output_test.c render/android/native_window_output.c render/rgb24_rgba8888.c -o $@

$(BUILD)/complex-plot-test: tests/complex_plot_test.c render/complex_plot.c render/wegert.c fourier/complex_field.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) $< render/complex_plot.c render/wegert.c fourier/complex_field.c -lm -o $@

test: $(BUILD)/complex-plot-test $(BUILD)/pcm-test $(BUILD)/backend-test $(BUILD)/output-backend-test $(BUILD)/speaker-input-test $(BUILD)/render-test $(BUILD)/fft-test $(BUILD)/framing-test $(BUILD)/voice-signal-test $(BUILD)/ick-polynomial-leaf-test $(BUILD)/rgb24-rgba8888-test $(BUILD)/native-window-output-test
	$(BUILD)/complex-plot-test
	$(BUILD)/pcm-test
	$(BUILD)/sparse-series-test
	$(BUILD)/backend-test
	$(BUILD)/output-backend-test
	$(BUILD)/speaker-input-test
	$(BUILD)/render-test $(BUILD)/fourier-render.ppm
	$(BUILD)/fft-test
	$(BUILD)/framing-test
	$(BUILD)/voice-signal-test
	$(BUILD)/ick-polynomial-leaf-test
	$(BUILD)/rgb24-rgba8888-test
	$(BUILD)/native-window-output-test

define android_abi
$(BUILD)/android/$(1)/glue.o: $(GLUE)/android_native_app_glue.c
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -fPIC $(3) -DANativeActivity_onCreate=fourier_glue_on_create -I$(GLUE) -c $$< -o $$@

$(BUILD)/android/$(1)/speaker_glue.o: $(GLUE)/android_native_app_glue.c
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -fPIC $(3) -I$(GLUE) -c $$< -o $$@

$(BUILD)/android/$(1)/render_glue.o: $(GLUE)/android_native_app_glue.c
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -fPIC $(3) -I$(GLUE) -c $$< -o $$@

$(BUILD)/android/$(1)/libfourier_render_ref.so: $(RENDER) $(HEADERS)
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -g $(WARN) $(3) $(INCLUDES) -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -shared -Wl,--no-undefined -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 $(RENDER) -lm -o $$@
	$(TOOLCHAIN)/llvm-readelf -h $$@

$(BUILD)/android/$(1)/staging/lib/$(1)/libfourier_microphone.so: android/native_main.c android/permission.c audio/android/aaudio_input.c audio/android/aaudio_output.c $(COMMON) $(HEADERS) $(BUILD)/android/$(1)/glue.o
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -g $(WARN) $(3) $(INCLUDES) -isystem $(GLUE) -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -shared -Wl,--no-undefined -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 android/native_main.c android/permission.c audio/android/aaudio_input.c audio/android/aaudio_output.c $(COMMON) $(BUILD)/android/$(1)/glue.o -laaudio -landroid -llog -lm -o $$@

$(BUILD)/android/$(1)/render-staging/lib/$(1)/libfourier_render_window.so: android/render_main.c render/android/native_window_output.c render/rgb24_rgba8888.c fourier/complex_field.c render/wegert.c render/complex_plot.c $(HEADERS) $(BUILD)/android/$(1)/render_glue.o
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -g $(WARN) $(3) $(INCLUDES) -isystem $(GLUE) -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -shared -Wl,--no-undefined -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 android/render_main.c render/android/native_window_output.c render/rgb24_rgba8888.c fourier/complex_field.c render/wegert.c render/complex_plot.c $(BUILD)/android/$(1)/render_glue.o -landroid -llog -lm -o $$@

$(BUILD)/android/$(1)/voice-staging/lib/$(1)/libfourier_voice.so: android/voice_main.c android/permission.c audio/android/aaudio_input.c audio/interface/pcm_ring.c audio/interface/audio_result.c fourier/pcm_block.c fourier/fft.c fourier/framing.c fourier/complex_field.c render/wegert.c render/complex_plot.c render/rgb24_rgba8888.c render/android/native_window_output.c $(HEADERS) $(BUILD)/android/$(1)/glue.o
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -g $(WARN) $(3) $(INCLUDES) -isystem $(GLUE) -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -shared -Wl,--no-undefined -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 android/voice_main.c android/permission.c audio/android/aaudio_input.c audio/interface/pcm_ring.c audio/interface/audio_result.c fourier/pcm_block.c fourier/fft.c fourier/framing.c fourier/complex_field.c render/wegert.c render/complex_plot.c render/rgb24_rgba8888.c render/android/native_window_output.c $(BUILD)/android/$(1)/glue.o -laaudio -landroid -llog -lm -o $$@

$(BUILD)/android/$(1)/speaker-staging/lib/$(1)/libfourier_speaker.so: android/speaker_main.c audio/android/aaudio_output.c audio/interface/speaker_input.c audio/interface/audio_result.c $(HEADERS) $(BUILD)/android/$(1)/speaker_glue.o
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -g $(WARN) $(3) $(INCLUDES) -isystem $(GLUE) -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -shared -Wl,--no-undefined -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 android/speaker_main.c audio/android/aaudio_output.c audio/interface/speaker_input.c audio/interface/audio_result.c $(BUILD)/android/$(1)/speaker_glue.o -laaudio -landroid -llog -lm -o $$@

$(BUILD)/fourier-microphone-$(1).apk: $(BUILD)/android/$(1)/staging/lib/$(1)/libfourier_microphone.so android/AndroidManifest.xml
	$(call package_native,org.isomorphismes.fouriersound,$(VERSION_CODE),0.1.0,android/AndroidManifest.xml,$(BUILD)/android/$(1)/staging/lib/$(1)/libfourier_microphone.so,$(1),$(BUILD)/fourier-microphone-$(1).apk)

$(BUILD)/fourier-render-window-$(1).apk: $(BUILD)/android/$(1)/render-staging/lib/$(1)/libfourier_render_window.so android/RenderManifest.xml
	$(call package_native,org.isomorphismes.fouriersound.render,$(VERSION_CODE),0.1.0,android/RenderManifest.xml,$(BUILD)/android/$(1)/render-staging/lib/$(1)/libfourier_render_window.so,$(1),$(BUILD)/fourier-render-window-$(1).apk)

$(BUILD)/fourier-voice-$(1).apk: $(BUILD)/android/$(1)/voice-staging/lib/$(1)/libfourier_voice.so android/VoiceManifest.xml
	$(call package_native,org.isomorphismes.fouriersound.voice,2,0.2.0,android/VoiceManifest.xml,$(BUILD)/android/$(1)/voice-staging/lib/$(1)/libfourier_voice.so,$(1),$(BUILD)/fourier-voice-$(1).apk)

$(BUILD)/fourier-speaker-$(1).apk: $(BUILD)/android/$(1)/speaker-staging/lib/$(1)/libfourier_speaker.so android/SpeakerManifest.xml
	$(call package_native,org.isomorphismes.fouriersound.speaker,$(VERSION_CODE),0.1.0,android/SpeakerManifest.xml,$(BUILD)/android/$(1)/speaker-staging/lib/$(1)/libfourier_speaker.so,$(1),$(BUILD)/fourier-speaker-$(1).apk)

endef

$(eval $(call android_abi,armeabi-v7a,armv7a-linux-androideabi26-clang,-marm -march=armv7-a))
$(eval $(call android_abi,arm64-v8a,aarch64-linux-android26-clang,))
$(eval $(call android_abi,x86_64,x86_64-linux-android26-clang,))

android: \
	$(BUILD)/fourier-microphone-armeabi-v7a.apk \
	$(BUILD)/fourier-microphone-arm64-v8a.apk \
	$(BUILD)/fourier-microphone-x86_64.apk \
	$(BUILD)/fourier-speaker-armeabi-v7a.apk \
	$(BUILD)/fourier-speaker-arm64-v8a.apk \
	$(BUILD)/fourier-speaker-x86_64.apk \
	$(BUILD)/fourier-render-window-armeabi-v7a.apk \
	$(BUILD)/fourier-render-window-arm64-v8a.apk \
	$(BUILD)/fourier-render-window-x86_64.apk \
	$(BUILD)/fourier-voice-armeabi-v7a.apk \
	$(BUILD)/fourier-voice-arm64-v8a.apk \
	$(BUILD)/fourier-voice-x86_64.apk \
	$(BUILD)/android/armeabi-v7a/libfourier_render_ref.so \
	$(BUILD)/android/arm64-v8a/libfourier_render_ref.so \
	$(BUILD)/android/x86_64/libfourier_render_ref.so


ICK_ARMV7_OBJECT ?= $(BUILD)/ick/armeabi-v7a/fourier_voice_leaf.o

$(BUILD)/android/armeabi-v7a/voice-ick-staging/lib/armeabi-v7a/libfourier_voice.so: android/voice_main.c android/permission.c audio/android/aaudio_input.c audio/interface/pcm_ring.c audio/interface/audio_result.c fourier/pcm_block.c fourier/fft.c fourier/framing.c fourier/complex_field.c render/wegert.c render/complex_plot.c render/rgb24_rgba8888.c render/android/native_window_output.c $(HEADERS) $(BUILD)/android/armeabi-v7a/glue.o $(ICK_ARMV7_OBJECT)
	mkdir -p $(@D)
	$(TOOLCHAIN)/armv7a-linux-androideabi26-clang -std=c17 -O2 -g $(WARN) -marm -march=armv7-a $(INCLUDES) -isystem $(GLUE) -DFOURIER_USE_ICK_POLYNOMIAL -DFOURIER_ICK_VERIFY -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -shared -Wl,--no-undefined -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 android/voice_main.c android/permission.c audio/android/aaudio_input.c audio/interface/pcm_ring.c audio/interface/audio_result.c fourier/pcm_block.c fourier/fft.c fourier/framing.c fourier/complex_field.c render/wegert.c render/complex_plot.c render/rgb24_rgba8888.c render/android/native_window_output.c $(BUILD)/android/armeabi-v7a/glue.o $(ICK_ARMV7_OBJECT) -laaudio -landroid -llog -lm -o $@

$(BUILD)/fourier-voice-ick-leaf-armeabi-v7a.apk: $(BUILD)/android/armeabi-v7a/voice-ick-staging/lib/armeabi-v7a/libfourier_voice.so android/VoiceManifest.xml
	$(call package_native,org.isomorphismes.fouriersound.voice,3,0.3.0-ick,android/VoiceManifest.xml,$(BUILD)/android/armeabi-v7a/voice-ick-staging/lib/armeabi-v7a/libfourier_voice.so,armeabi-v7a,$(BUILD)/fourier-voice-ick-leaf-armeabi-v7a.apk)

android-ick-leaf-armv7: $(BUILD)/fourier-voice-ick-leaf-armeabi-v7a.apk


# Shipping-sized MIRO A1 packages.  Keep debug information as separate build
# artifacts, strip only the copy that is placed in the APK.
$(BUILD)/release/ndk/lib/armeabi-v7a/libfourier_voice.so: $(BUILD)/android/armeabi-v7a/voice-staging/lib/armeabi-v7a/libfourier_voice.so
	mkdir -p $(@D) $(BUILD)/symbols/ndk
	$(TOOLCHAIN)/llvm-objcopy --only-keep-debug $< $(BUILD)/symbols/ndk/libfourier_voice.so.debug
	cp $< $@
	$(TOOLCHAIN)/llvm-strip --strip-unneeded $@
	! $(TOOLCHAIN)/llvm-readelf -S $@ | grep -q '\.debug_'
	$(TOOLCHAIN)/llvm-readelf -h $@

$(BUILD)/fourier-voice-miro-release.apk: $(BUILD)/release/ndk/lib/armeabi-v7a/libfourier_voice.so android/VoiceManifest.xml
	$(call package_native,org.isomorphismes.fouriersound.voice,5,0.5.0,android/VoiceManifest.xml,$(BUILD)/release/ndk/lib/armeabi-v7a/libfourier_voice.so,armeabi-v7a,$(BUILD)/fourier-voice-miro-release.apk)

$(BUILD)/android/armeabi-v7a/voice-ick-release-unstripped/lib/armeabi-v7a/libfourier_voice.so: android/voice_main.c android/permission.c audio/android/aaudio_input.c audio/interface/pcm_ring.c audio/interface/audio_result.c fourier/pcm_block.c fourier/fft.c fourier/framing.c fourier/complex_field.c render/wegert.c render/complex_plot.c render/rgb24_rgba8888.c render/android/native_window_output.c $(HEADERS) $(BUILD)/android/armeabi-v7a/glue.o $(ICK_ARMV7_OBJECT)
	mkdir -p $(@D)
	$(TOOLCHAIN)/armv7a-linux-androideabi26-clang -std=c17 -O2 -g $(WARN) -marm -march=armv7-a $(INCLUDES) -isystem $(GLUE) -DFOURIER_USE_ICK_POLYNOMIAL -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -shared -Wl,--no-undefined -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 android/voice_main.c android/permission.c audio/android/aaudio_input.c audio/interface/pcm_ring.c audio/interface/audio_result.c fourier/pcm_block.c fourier/fft.c fourier/framing.c fourier/complex_field.c render/wegert.c render/complex_plot.c render/rgb24_rgba8888.c render/android/native_window_output.c $(BUILD)/android/armeabi-v7a/glue.o $(ICK_ARMV7_OBJECT) -laaudio -landroid -llog -lm -o $@

$(BUILD)/release/ick/lib/armeabi-v7a/libfourier_voice.so: $(BUILD)/android/armeabi-v7a/voice-ick-release-unstripped/lib/armeabi-v7a/libfourier_voice.so
	mkdir -p $(@D) $(BUILD)/symbols/ick
	$(TOOLCHAIN)/llvm-objcopy --only-keep-debug $< $(BUILD)/symbols/ick/libfourier_voice.so.debug
	cp $< $@
	$(TOOLCHAIN)/llvm-strip --strip-unneeded $@
	! $(TOOLCHAIN)/llvm-readelf -S $@ | grep -q '\.debug_'
	$(TOOLCHAIN)/llvm-readelf -h $@

$(BUILD)/fourier-voice-ick-leaf-miro-release.apk: $(BUILD)/release/ick/lib/armeabi-v7a/libfourier_voice.so android/VoiceManifest.xml
	$(call package_native,org.isomorphismes.fouriersound.voice,5,0.5.0,android/VoiceManifest.xml,$(BUILD)/release/ick/lib/armeabi-v7a/libfourier_voice.so,armeabi-v7a,$(BUILD)/fourier-voice-ick-leaf-miro-release.apk)

android-miro-release: $(BUILD)/fourier-voice-miro-release.apk
android-ick-leaf-miro-release: $(BUILD)/fourier-voice-ick-leaf-miro-release.apk
miro-release-compare: android-miro-release android-ick-leaf-miro-release

# GPU candidate: upload framed PCM, compute FFT in SSBOs, render without spectrum readback.
GPU_VOICE_SOURCES = android/gpu_voice_main.c android/permission.c \
	audio/android/aaudio_input.c audio/interface/pcm_ring.c \
	audio/interface/audio_result.c fourier/pcm_block.c fourier/framing.c \
	render/android/gpu_voice_pipeline.c

$(BUILD)/android/armeabi-v7a/voice-gpu-unstripped/lib/armeabi-v7a/libfourier_voice.so: $(GPU_VOICE_SOURCES) $(HEADERS) $(BUILD)/android/armeabi-v7a/glue.o
	mkdir -p $(@D)
	$(TOOLCHAIN)/armv7a-linux-androideabi26-clang -std=c17 -O2 -g $(WARN) \
		-marm -march=armv7-a $(INCLUDES) -isystem $(GLUE) \
		-fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -shared \
		-Wl,--no-undefined -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 \
		$(GPU_VOICE_SOURCES) $(BUILD)/android/armeabi-v7a/glue.o \
		-laaudio -landroid -llog -lEGL -lGLESv3 -lm -o $@

$(BUILD)/release/gpu/lib/armeabi-v7a/libfourier_voice.so: $(BUILD)/android/armeabi-v7a/voice-gpu-unstripped/lib/armeabi-v7a/libfourier_voice.so
	mkdir -p $(@D) $(BUILD)/symbols/gpu
	$(TOOLCHAIN)/llvm-objcopy --only-keep-debug $< $(BUILD)/symbols/gpu/libfourier_voice.so.debug
	cp $< $@
	$(TOOLCHAIN)/llvm-strip --strip-unneeded $@
	! $(TOOLCHAIN)/llvm-readelf -S $@ | grep -q '\.debug_'
	$(TOOLCHAIN)/llvm-readelf -h $@

$(BUILD)/fourier-voice-gpu-miro-release.apk: $(BUILD)/release/gpu/lib/armeabi-v7a/libfourier_voice.so android/GpuVoiceManifest.xml
	$(call package_native,org.isomorphismes.fouriersound.voice,5,0.5.0-gpu,android/GpuVoiceManifest.xml,$<,armeabi-v7a,$@)

.PHONY: android-gpu-miro-release
android-gpu-miro-release: $(BUILD)/fourier-voice-gpu-miro-release.apk
