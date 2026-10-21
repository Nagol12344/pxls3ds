#include "core/config.h"
#include "core/log.h"
#include "network/http_client.h"

#include <3ds.h>
#include <malloc.h>

#include <cstdio>

namespace {

constexpr size_t SOC_SHAREDMEM_SIZE = 0x100000;
constexpr size_t SOC_SHAREDMEM_ALIGN = 0x1000;

u32 *socSharedMem = nullptr;

bool initNetwork() {
	socSharedMem = static_cast<u32 *>(memalign(SOC_SHAREDMEM_ALIGN, SOC_SHAREDMEM_SIZE));
	if (!socSharedMem) {
		printf("[NET] Failed to allocate SOC shared memory\n");
		return false;
	}

	Result rc = socInit(socSharedMem, SOC_SHAREDMEM_SIZE);
	if (R_FAILED(rc)) {
		printf("[NET] socInit failed: 0x%08lX\n", static_cast<unsigned long>(rc));
		free(socSharedMem);
		socSharedMem = nullptr;
		return false;
	}

	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		printf("[NET] curl_global_init failed: %s\n", curl_easy_strerror(res));
		socExit();
		free(socSharedMem);
		socSharedMem = nullptr;
		return false;
	}

	printf("[NET] Network ready\n");
	return true;
}

void exitNetwork() {
	curl_global_cleanup();

	if (socSharedMem) {
		socExit();
		free(socSharedMem);
		socSharedMem = nullptr;
	}
}

} // namespace

int main(int argc, char **argv) {
	osSetSpeedupEnable(true);
	gfxInitDefault();
	consoleInit(GFX_TOP, nullptr);

	printf("\x1b[2;2HPxls3DS\n");

	if (R_SUCCEEDED(romfsInit())) {
		printf("[NET] RomFS mounted (CA certificates available)\n");
	} else {
		printf("[NET] romfsInit failed, HTTPS certificate verification may fail\n");
	}

	Logger::init();
	Config::getInstance().load();

	if (initNetwork()) {
		// Startup connectivity check (remove once the real UI is in place).
		printf("[NET] Testing HTTP request...\n");

		Network::HttpClient client;
		client.setTimeout(5);
		Network::HttpResponse response = client.get("https://example.com");

		if (response.success) {
			printf("[NET] HTTP %ld (%zu bytes)\n", response.statusCode, response.body.size());
		} else {
			printf("[NET] Request failed: %s (HTTP %ld)\n",
			       response.error.empty() ? "unknown error" : response.error.c_str(), response.statusCode);
		}
	}

	printf("\x1b[28;2HPress Start to exit.\n");

	while (aptMainLoop()) {
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	exitNetwork();
	romfsExit();
	gfxExit();
	return 0;
}
