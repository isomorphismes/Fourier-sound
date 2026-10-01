# Recipes are direct build-tool invocations using make's fixed shell interface.
# No app Java/Kotlin, Gradle, or maintained shell program.
BUILD ?= build
CC ?= cc
CFLAGS ?= -O2 -g
WARN = -Wall -Wextra -Werror -Wpedantic -Wshadow
INCLUDES = -Iaudio/interface -Iaudio/android -Ifourier -Iacceptance
COMMON = audio/interface/pcm_ring.c audio/interface/audio_result.c fourier/pcm_block.c acceptance/microphone_check.c
HEADERS = $(wildcard audio/interface/*.h audio/android/*.h fourier/*.h acceptance/*.h)
ANDROID_HOME ?= /opt/android-sdk
NDK ?= $(ANDROID_HOME)/ndk/27.2.12479018
TOOLCHAIN = $(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin
GLUE = $(NDK)/sources/android/native_app_glue
TOOLS = $(ANDROID_HOME)/build-tools/36.0.0
ANDROID_JAR = $(ANDROID_HOME)/platforms/android-36/android.jar
ANDROID_KEYSTORE ?= .test-signing/_/build/app/wegert-debug.keystore
VERSION_CODE ?= 1

SOUNDS_REPO ?= https://github.com/dilapidated-shed/sounds.git
SOUNDS_REV ?= d4fcf6f80a7dc57375f3aa873560f9ea36cdf3af
SOUNDS_DIR = $(BUILD)/sounds

MEDIA_DIR = $(BUILD)/media
MEDIA_RATE = 44100
MEDIA_SECONDS = 8
MEDIA_FIXTURES = \
	$(MEDIA_DIR)/thunder-rain.s16 \
	$(MEDIA_DIR)/iceberg-contact.s16 \
	$(MEDIA_DIR)/dvorak-largo.s16 \
	$(MEDIA_DIR)/bartok-sonatina.s16 \
	$(MEDIA_DIR)/russolo-corale.s16 \
	$(MEDIA_DIR)/russolo-serenata.s16

.PHONY: all test android media-fetch media-test
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

test: $(BUILD)/pcm-test $(BUILD)/backend-test $(BUILD)/output-backend-test $(BUILD)/speaker-input-test
	$(BUILD)/pcm-test
	$(BUILD)/backend-test
	$(BUILD)/output-backend-test
	$(BUILD)/speaker-input-test


$(SOUNDS_DIR)/.ready:
	rm -rf $(SOUNDS_DIR).tmp $(SOUNDS_DIR)
	git clone --quiet '$(SOUNDS_REPO)' $(SOUNDS_DIR).tmp
	git -C $(SOUNDS_DIR).tmp checkout --quiet --detach '$(SOUNDS_REV)'
	cd $(SOUNDS_DIR).tmp && sh scripts/fetch-public-audio.sh
	mv $(SOUNDS_DIR).tmp $(SOUNDS_DIR)
	touch $@

$(MEDIA_DIR)/thunder-rain.s16: $(SOUNDS_DIR)/.ready
	mkdir -p $(@D)
	source=$$(awk -F '\t' '$$1 == "thunder-rain-veranda" {print $$2}' $(SOUNDS_DIR)/manifest.tsv); test -n "$$source"; ffmpeg -nostdin -loglevel error -y -i "$(SOUNDS_DIR)/$$source" -t $(MEDIA_SECONDS) -ac 1 -ar $(MEDIA_RATE) -f s16le $@

$(MEDIA_DIR)/iceberg-contact.s16: $(SOUNDS_DIR)/.ready
	mkdir -p $(@D)
	source=$$(awk -F '\t' '$$1 == "noaa-iceberg-harmonic-tremor" {print $$2}' $(SOUNDS_DIR)/manifest.tsv); test -n "$$source"; ffmpeg -nostdin -loglevel error -y -i "$(SOUNDS_DIR)/$$source" -t $(MEDIA_SECONDS) -ac 1 -ar $(MEDIA_RATE) -f s16le $@

$(MEDIA_DIR)/dvorak-largo.s16: $(SOUNDS_DIR)/.ready
	mkdir -p $(@D)
	source=$$(awk -F '\t' '$$1 == "dvorak-new-world-largo" {print $$2}' $(SOUNDS_DIR)/manifest.tsv); test -n "$$source"; ffmpeg -nostdin -loglevel error -y -ss 60 -i "$(SOUNDS_DIR)/$$source" -t $(MEDIA_SECONDS) -ac 1 -ar $(MEDIA_RATE) -f s16le $@

$(MEDIA_DIR)/bartok-sonatina.s16: $(SOUNDS_DIR)/.ready
	mkdir -p $(@D)
	source=$$(awk -F '\t' '$$1 == "bartok-sonatina" {print $$2}' $(SOUNDS_DIR)/manifest.tsv); test -n "$$source"; ffmpeg -nostdin -loglevel error -y -ss 10 -i "$(SOUNDS_DIR)/$$source" -t $(MEDIA_SECONDS) -ac 1 -ar $(MEDIA_RATE) -f s16le $@

$(MEDIA_DIR)/russolo-corale.s16: $(SOUNDS_DIR)/.ready
	mkdir -p $(@D)
	source=$$(awk -F '\t' '$$1 == "russolo-corale" {print $$2}' $(SOUNDS_DIR)/manifest.tsv); test -n "$$source"; ffmpeg -nostdin -loglevel error -y -i "$(SOUNDS_DIR)/$$source" -t $(MEDIA_SECONDS) -ac 1 -ar $(MEDIA_RATE) -f s16le $@

$(MEDIA_DIR)/russolo-serenata.s16: $(SOUNDS_DIR)/.ready
	mkdir -p $(@D)
	source=$$(awk -F '\t' '$$1 == "russolo-serenata" {print $$2}' $(SOUNDS_DIR)/manifest.tsv); test -n "$$source"; ffmpeg -nostdin -loglevel error -y -i "$(SOUNDS_DIR)/$$source" -t $(MEDIA_SECONDS) -ac 1 -ar $(MEDIA_RATE) -f s16le $@

$(BUILD)/media-fixture-test: tests/media_fixture_test.c fourier/pcm_block.c audio/interface/speaker_input.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -std=c17 $(CFLAGS) $(WARN) $(INCLUDES) tests/media_fixture_test.c fourier/pcm_block.c audio/interface/speaker_input.c -lm -o $@

media-fetch: $(MEDIA_FIXTURES)

media-test: media-fetch $(BUILD)/media-fixture-test
	$(BUILD)/media-fixture-test $(MEDIA_FIXTURES)

define android_abi
$(BUILD)/android/$(1)/glue.o: $(GLUE)/android_native_app_glue.c
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -fPIC $(3) -DANativeActivity_onCreate=fourier_glue_on_create -I$(GLUE) -c $$< -o $$@

$(BUILD)/android/$(1)/speaker_glue.o: $(GLUE)/android_native_app_glue.c
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -fPIC $(3) -I$(GLUE) -c $$< -o $$@

$(BUILD)/android/$(1)/staging/lib/$(1)/libfourier_microphone.so: android/native_main.c android/permission.c audio/android/aaudio_input.c audio/android/aaudio_output.c $(COMMON) $(HEADERS) $(BUILD)/android/$(1)/glue.o
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -g $(WARN) $(3) $(INCLUDES) -isystem $(GLUE) -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -shared -Wl,--no-undefined -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 android/native_main.c android/permission.c audio/android/aaudio_input.c audio/android/aaudio_output.c $(COMMON) $(BUILD)/android/$(1)/glue.o -laaudio -landroid -llog -lm -o $$@

$(BUILD)/android/$(1)/speaker-staging/lib/$(1)/libfourier_speaker.so: android/speaker_main.c audio/android/aaudio_output.c audio/interface/speaker_input.c audio/interface/audio_result.c $(HEADERS) $(BUILD)/android/$(1)/speaker_glue.o
	mkdir -p $$(@D)
	$(TOOLCHAIN)/$(2) -std=c17 -O2 -g $(WARN) $(3) $(INCLUDES) -isystem $(GLUE) -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -shared -Wl,--no-undefined -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 android/speaker_main.c audio/android/aaudio_output.c audio/interface/speaker_input.c audio/interface/audio_result.c $(BUILD)/android/$(1)/speaker_glue.o -laaudio -landroid -llog -lm -o $$@

$(BUILD)/fourier-microphone-$(1).apk: $(BUILD)/android/$(1)/staging/lib/$(1)/libfourier_microphone.so android/AndroidManifest.xml
	$(TOOLS)/aapt2 link -I $(ANDROID_JAR) --manifest android/AndroidManifest.xml --min-sdk-version 26 --target-sdk-version 36 --version-code $(VERSION_CODE) --version-name 0.1.0 -o $(BUILD)/android/$(1)/unsigned.apk
	cd $(BUILD)/android/$(1)/staging && zip -0 -q -r ../unsigned.apk lib
	$(TOOLS)/zipalign -f -P 16 4 $(BUILD)/android/$(1)/unsigned.apk $(BUILD)/android/$(1)/aligned.apk
	$(TOOLS)/apksigner sign --ks $(ANDROID_KEYSTORE) --ks-key-alias wegert-debug --ks-pass pass:wegert-debug --key-pass pass:wegert-debug --out $$@ $(BUILD)/android/$(1)/aligned.apk
	$(TOOLS)/apksigner verify --verbose --print-certs $$@ > $(BUILD)/android/$(1)/signer.txt
	grep -Fq 'Signer #1 certificate SHA-256 digest: de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9' $(BUILD)/android/$(1)/signer.txt
	$(TOOLS)/zipalign -c -P 16 4 $$@
	unzip -Z1 $$@ > $(BUILD)/android/$(1)/entries.txt
	! grep -E '(^|/)classes[0-9]*\.dex$$$$' $(BUILD)/android/$(1)/entries.txt
	$(TOOLCHAIN)/llvm-readelf -h $(BUILD)/android/$(1)/staging/lib/$(1)/libfourier_microphone.so
	sha256sum $$@ > $$@.sha256

$(BUILD)/fourier-speaker-$(1).apk: $(BUILD)/android/$(1)/speaker-staging/lib/$(1)/libfourier_speaker.so android/SpeakerManifest.xml
	$(TOOLS)/aapt2 link -I $(ANDROID_JAR) --manifest android/SpeakerManifest.xml --min-sdk-version 26 --target-sdk-version 36 --version-code $(VERSION_CODE) --version-name 0.1.0 -o $(BUILD)/android/$(1)/speaker-unsigned.apk
	cd $(BUILD)/android/$(1)/speaker-staging && zip -0 -q -r ../speaker-unsigned.apk lib
	$(TOOLS)/zipalign -f -P 16 4 $(BUILD)/android/$(1)/speaker-unsigned.apk $(BUILD)/android/$(1)/speaker-aligned.apk
	$(TOOLS)/apksigner sign --ks $(ANDROID_KEYSTORE) --ks-key-alias wegert-debug --ks-pass pass:wegert-debug --key-pass pass:wegert-debug --out $$@ $(BUILD)/android/$(1)/speaker-aligned.apk
	$(TOOLS)/apksigner verify --verbose --print-certs $$@ > $(BUILD)/android/$(1)/speaker-signer.txt
	grep -Fq 'Signer #1 certificate SHA-256 digest: de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9' $(BUILD)/android/$(1)/speaker-signer.txt
	$(TOOLS)/zipalign -c -P 16 4 $$@
	unzip -Z1 $$@ > $(BUILD)/android/$(1)/speaker-entries.txt
	! grep -E '(^|/)classes[0-9]*\.dex$$$$' $(BUILD)/android/$(1)/speaker-entries.txt
	$(TOOLCHAIN)/llvm-readelf -h $(BUILD)/android/$(1)/speaker-staging/lib/$(1)/libfourier_speaker.so
	sha256sum $$@ > $$@.sha256
endef

$(eval $(call android_abi,armeabi-v7a,armv7a-linux-androideabi26-clang,-mthumb -march=armv7-a))
$(eval $(call android_abi,arm64-v8a,aarch64-linux-android26-clang,))
$(eval $(call android_abi,x86_64,x86_64-linux-android26-clang,))

android: \
	$(BUILD)/fourier-microphone-armeabi-v7a.apk \
	$(BUILD)/fourier-microphone-arm64-v8a.apk \
	$(BUILD)/fourier-microphone-x86_64.apk \
	$(BUILD)/fourier-speaker-armeabi-v7a.apk \
	$(BUILD)/fourier-speaker-arm64-v8a.apk \
	$(BUILD)/fourier-speaker-x86_64.apk
