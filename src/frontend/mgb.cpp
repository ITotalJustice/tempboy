#include "mgb.hpp"
#include "../core/gb.h"
#include "io/romloader.hpp"
#include "io/ifile_cfile.hpp"
#include "util/util.hpp"
#include "util/log.hpp"

#include "nativefiledialog/nfd.hpp"
#ifdef USE_DISCORD
#include <discord_rpc/discord_rpc.h>
#endif // USE_DISCORD

#include <cstring>
#include <cassert>
#include <fstream>

#ifdef USE_DISCORD
static const char* APPLICATION_ID = "822897241202884618";

static void UpdatePresence() {
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.state = "Debug";
    discordPresence.details = "Coding away...";
    discordPresence.startTimestamp = 1507665886;
    discordPresence.endTimestamp = 1507665886;
    discordPresence.largeImageKey = "test";
    discordPresence.largeImageText = "TotalGB";
    discordPresence.smallImageKey = "test";
    discordPresence.smallImageText = "Rogue - Level 100";
    discordPresence.partyId = "ae488379-351d-4a4f-ad32-2b9b01c91657";
    discordPresence.partySize = 0;
    discordPresence.partyMax = 5;
    discordPresence.joinSecret = "MTI4NzM0OjFpMmhuZToxMjMxMjM= ";
    Discord_UpdatePresence(&discordPresence);
}

static void handleDiscordReady(const DiscordUser* connectedUser) {
    printf("\nDiscord: connected to user %s#%s - %s\n",
           connectedUser->username,
           connectedUser->discriminator,
           connectedUser->userId);
}

static void handleDiscordDisconnected(int errcode, const char* message) {
    printf("\nDiscord: disconnected (%d: %s)\n", errcode, message);
}

static void handleDiscordError(int errcode, const char* message) {
    printf("\nDiscord: error (%d: %s)\n", errcode, message);
}

static void handleDiscordJoin(const char* secret) {
    printf("\nDiscord: join (%s)\n", secret);
}

static void handleDiscordSpectate(const char* secret) {
    printf("\nDiscord: spectate (%s)\n", secret);
}

static void handleDiscordJoinRequest(const DiscordUser* request) {
    int response = -1;
    // response = DISCORD_REPLY_NO;
    response = DISCORD_REPLY_YES;

    if (response != -1) {
        Discord_Respond(request->userId, response);
    }
}

static void discordInit() {
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handleDiscordReady;
    handlers.disconnected = handleDiscordDisconnected;
    handlers.errored = handleDiscordError;
    handlers.joinGame = handleDiscordJoin;
    handlers.spectateGame = handleDiscordSpectate;
    handlers.joinRequest = handleDiscordJoinRequest;
    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
}
#endif // USE_DISCORD

namespace mgb {

// vblank callback is to let the frontend know that it should render the screen.
// This removes any screen tearing and changes to the pixels whilst in vblank.
auto VblankCallback(GB_Data*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnVblankCallback();
}

auto HblankCallback(GB_Data*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnHblankCallback();
}

auto DmaCallback(GB_Data*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnDmaCallback();
}

auto HaltCallback(GB_Data*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnHaltCallback();
}

auto StopCallback(GB_Data*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnStopCallback();
}

// error callback is to be used to handle errors that may happen during emulation.
// these will be updated with warnings also,
// such as trying to write to a rom or OOB read / write from the game.
auto ErrorCallback(GB_Data*, void* user, struct GB_ErrorData* data) -> void {
    assert(user);
    auto* app = static_cast<Instance*>(user);
    app->OnErrorCallback(data);
}

auto Instance::OnVblankCallback() -> void {
    void* pixles; int pitch;

    SDL_LockTexture(texture, NULL, &pixles, &pitch);
    std::memcpy(pixles, gameboy->ppu.pixles, sizeof(gameboy->ppu.pixles));
    SDL_UnlockTexture(texture);
}

auto Instance::OnHblankCallback() -> void {

}

auto Instance::OnDmaCallback() -> void {

}

auto Instance::OnHaltCallback() -> void {

}

auto Instance::OnStopCallback() -> void {
    printf("[ERROR] cpu stop instruction called!\n");
}

auto Instance::OnErrorCallback(struct GB_ErrorData* data) -> void {
    switch (data->type) {
        case GB_ErrorType::GB_ERROR_TYPE_UNKNOWN_INSTRUCTION:
            printf("[ERROR] UNK instruction OP 0x%02X CB: %s\n", data->unk_instruction.opcode, data->unk_instruction.cb_prefix ? "TRUE" : "FALSE");
            break;

        case GB_ErrorType::GB_ERROR_TYPE_INFO:
            printf("[INFO] %s\n", data->info.message);
            break;

        case GB_ErrorType::GB_ERROR_TYPE_WARN:
            printf("[WARN] %s\n", data->warn.message);
            break;

        case GB_ErrorType::GB_ERROR_TYPE_ERROR:
            printf("[ERROR] %s\n", data->error.message);
            break;

        case GB_ErrorType::GB_ERROR_TYPE_UNK:
            printf("[ERROR] Unknown gb error...\n");
            break;
    }
}

auto Instance::LoadRom(const std::string& path) -> bool {
    // if we have a rom alread loaded, try and save the game
    // first before exiting...
    if (this->HasRom()) {
        this->SaveGame(this->rom_path);
    } else {
        this->gameboy = std::make_unique<GB_Data>();
        GB_init(this->gameboy.get());

        GB_set_vblank_callback(this->gameboy.get(), VblankCallback, this);
        GB_set_hblank_callback(this->gameboy.get(), HblankCallback, this);
        GB_set_dma_callback(this->gameboy.get(), DmaCallback, this);
        GB_set_halt_callback(this->gameboy.get(), HaltCallback, this);
        GB_set_stop_callback(this->gameboy.get(), StopCallback, this);
        GB_set_error_callback(this->gameboy.get(), ErrorCallback, this);
    }

    io::RomLoader romloader{path};
    if (!romloader.good()) {
        return false;
    }

    // get the size
    const auto file_size = romloader.size();


    // resize vector
    this->rom_data.resize(file_size);

    // read entire file...
    romloader.read(this->rom_data.data(), this->rom_data.size());

    if (-1 == GB_loadrom_data(this->gameboy.get(), this->rom_data.data(), this->rom_data.size())) {
        return false;
    }

    // save the path and set that the rom had loaded!
    this->rom_path = path;

    if (!this->HasWindow()) {
        this->CreateWindow();
    }

    // try and set the rom name in window title
    {
        struct GB_CartName cart_name;
        if (0 == GB_get_rom_name(this->gameboy.get(), &cart_name)) {
            SDL_SetWindowTitle(this->window, cart_name.name);
        }
    }

    // try and load a savefile (if any...)
    this->LoadSave(this->rom_path);

    return true;
}

auto Instance::CloseRom(bool close_window) -> bool {
    if (this->HasRom()) {
        this->SaveGame(this->rom_path);
    }

    if (close_window && this->HasWindow()) {
        this->gameboy.reset();
        this->DestroyWindow();
    }

    return true;
}

auto Instance::SaveGame(const std::string& path) -> void {
    if (GB_has_save(this->gameboy.get()) || GB_has_rtc(this->gameboy.get())) {
        struct GB_SaveData save_data;
        GB_savegame(this->gameboy.get(), &save_data);
        
        // save sram
        if (save_data.size > 0) {
            const auto save_path = util::getSavePathFromString(path);
            io::Cfile file{save_path, "wb"};
            if (file.good()) {
                file.write(save_data.data, save_data.size);
            }
        }

        // save rtc
        if (save_data.has_rtc == GB_TRUE) {
            const auto save_path = util::getRtcPathFromString(path);
            io::Cfile file{save_path, "wb"};
            if (file.good()) {
                file.write((u8*)&save_data.rtc, sizeof(save_data.rtc));
            }
        }
    }
}

auto Instance::LoadSave(const std::string& path) -> void {
    struct GB_SaveData save_data{};

    // load sram
    if (GB_has_save(this->gameboy.get())) {
        printf("has save!\n");

        const auto save_path = util::getSavePathFromString(path);
        const auto save_size = GB_calculate_savedata_size(this->gameboy.get());
        
        io::Cfile file{save_path, "rb"};

        if (file.good() && file.size() == save_size) {
            printf("trying to read... %u\n", save_size);
            file.read(save_data.data, save_size);
            save_data.size = save_size;
        }
    }
    
    // load rtc
    if (GB_has_rtc(this->gameboy.get())) {
        const auto save_path = util::getRtcPathFromString(path);
        io::Cfile file{save_path, "rb"};

        if (file.good() && file.size() == sizeof(save_data.rtc)) {
            file.read((u8*)&save_data.rtc, sizeof(save_data.rtc));
            save_data.has_rtc = GB_TRUE;
        }
    }

    GB_loadsave(this->gameboy.get(), &save_data);
}

auto Instance::CreateWindow() -> void {
    this->window = SDL_CreateWindow("gb-emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 2, 144 * 2, SDL_WINDOW_SHOWN);
    this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    SDL_SetWindowMinimumSize(this->window, 160, 144);
}

auto Instance::DestroyWindow() -> void {
    SDL_DestroyTexture(this->texture);
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyWindow(this->window);

    this->texture = nullptr;
    this->renderer = nullptr;
    this->window = nullptr;
}

auto Instance::HasWindow() const -> bool {
    return this->window != nullptr;
}

auto Instance::HasRom() const -> bool {
    return this->gameboy != nullptr;
}

auto Instance::GetGB() -> GB_Data* {
    return this->gameboy.get();
}

App::App() {
#ifdef USE_DISCORD
    discordInit();
#endif

    {
        SDL_version compiled;
        SDL_version linked;

        SDL_VERSION(&compiled);
        SDL_GetVersion(&linked);
        printf("We compiled against SDL version %d.%d.%d ...\n",
            compiled.major, compiled.minor, compiled.patch);
        printf("But we are linking against SDL version %d.%d.%d.\n",
            linked.major, linked.minor, linked.patch);
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerAddMappingsFromFile("res/mappings/gamecontrollerdb.txt");
}

App::~App() {
    for (auto& p : this->emu_instances) {
        // we want to close the window as well this time...
        p.CloseRom(true);
    }

    // cleanup joysticks and controllers
    for (auto &p : this->joysticks) {
        SDL_JoystickClose(p.ptr);
        p.ptr = nullptr;
    }

    for (auto &p : this->controllers) {
        SDL_GameControllerClose(p.ptr);
        p.ptr = nullptr;
    }

	SDL_Quit();

#ifdef USE_DISCORD
    Discord_Shutdown();
#endif
}

auto App::LoadRom(const std::string& path, bool dual) -> bool {
    // load the game in slot 1
    if (dual == false) {
        if (this->emu_instances[0].LoadRom(path)) {
            this->run_state = EmuRunState::SINGLE;
            return true;
        }
    } else { // else slot 2
        if (this->emu_instances[1].LoadRom(path)) {
            this->run_state = EmuRunState::DUAL;
            // connect both games via link cable
            GB_connect_link_cable_builtin(this->emu_instances[0].GetGB(), this->emu_instances[1].GetGB());
            GB_connect_link_cable_builtin(this->emu_instances[1].GetGB(), this->emu_instances[0].GetGB());
            return true;
        }
    }

    this->run_state = EmuRunState::NONE;
    return false;
}

auto App::FilePicker() -> void {
    // initialize NFD
    NFD::Guard nfdGuard;

    // auto-freeing memory
    NFD::UniquePath outPath;

    // prepare filters for the dialog
    const nfdfilteritem_t filterItem[2] = { { "Roms", "gb,gbc" },{ "Zip", "zip,gzip" } };

    // show the dialog
    const auto result = NFD::OpenDialog(outPath, filterItem, sizeof(filterItem) / sizeof(filterItem[0]));

    switch (result) {
        case NFD_ERROR:
            printf("[ERROR] failed to open file...\n");
            break;

        case NFD_CANCEL:
            printf("[INFO] cancled open file request...\n");
            break;

        case NFD_OKAY:
            this->LoadRom(outPath.get());
            break;
    }
}

auto App::HasController(int which) const -> bool {
    for (auto &p : this->controllers) {
        if (p.id == which) {
            return true;
        }
    }
    return false;
}

auto App::AddController(int index) -> bool {
    auto controller = SDL_GameControllerOpen(index);
    if (!controller) {
        return log::errReturn(false, "Failed to open controller from index %d\n", index);
    }

    ControllerCtx ctx;
    ctx.ptr = controller;
    ctx.id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
    this->controllers.push_back(std::move(ctx));

    return true;
}

auto App::ResizeScreen() -> void {

}

void GB_run_frames(struct GB_Data* gb1, struct GB_Data* gb2) {
	GB_U32 cycles_elapsed1 = 0;
    GB_U32 cycles_elapsed2 = 0;

    while (cycles_elapsed1 < GB_FRAME_CPU_CYCLES || cycles_elapsed2 < GB_FRAME_CPU_CYCLES) {
        if (cycles_elapsed1 < GB_FRAME_CPU_CYCLES) {
            cycles_elapsed1 += GB_run_step(gb1);
        }
        if (cycles_elapsed2 < GB_FRAME_CPU_CYCLES) {
            cycles_elapsed2 += GB_run_step(gb2);
        }
    }
}

auto App::Loop() -> void {
    while (this->running) {
        // handle sdl2 events
        this->Events();

        switch (this->run_state) {
            case EmuRunState::NONE:
                break;

            case EmuRunState::SINGLE:
                GB_run_frame(this->emu_instances[0].GetGB());
                break;

            case EmuRunState::DUAL:
                GB_run_frame(this->emu_instances[0].GetGB());
                GB_run_frame(this->emu_instances[1].GetGB());
                // GB_run_frames(
                //     this->emu_instances[0].GetGB(),
                //     this->emu_instances[1].GetGB()
                // );
                break;
        }

        // render the screen
        this->Draw();

    #ifdef USE_DISCORD
        UpdatePresence();
    #ifdef DISCORD_DISABLE_IO_THREAD
        Discord_UpdateConnection();
    #endif // DISCORD_DISABLE_IO_THREAD
        Discord_RunCallbacks();
    #endif // USE_DISCORD
    }
}

auto App::Draw() -> void {
    for (auto& p : this->emu_instances) {
        SDL_RenderClear(p.renderer);
        SDL_RenderCopy(p.renderer, p.texture, NULL, NULL);
        SDL_RenderPresent(p.renderer);
    }
}

} // namespace mgb
