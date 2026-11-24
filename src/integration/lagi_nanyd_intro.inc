#include "lagi/cutscene_audio.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"

namespace {

struct NanydDialogue : s_workAreaTemplate<NanydDialogue>
{
    std::vector<std::uint8_t>* pcm;
    bool displayed;

    static void Update(NanydDialogue* self)
    {
        if (self->pcm)
        {
            const unsigned rate = lagi::disc::selected_disc() == 4 ? 18000 : 22050;
            const int result = lagiSubmitCutscenePCM(
                self, self->pcm->data(), self->pcm->size(), rate, 1, 16);
            if (result == -1)
                return;
            delete self->pcm;
            self->pcm = nullptr;
            if (result != 0)
            {
                lagi::platform::logging::writef("[NanydDialogue] invalid PCM\n");
                self->getTask()->markFinished();
            }
            return;
        }

        if (lagiCutscenePCMQueued(self) == 0)
            self->getTask()->markFinished();
    }

    static void Delete(NanydDialogue* self)
    {
        lagiStopCutscenePCM(self);
        delete self->pcm;
        if (self->displayed)
        {
            setupVDP2StringRendering(3, 25, 38, 4);
            clearVdp2TextArea();
        }
    }
};

void lagiStartBattleDialogue(p_workArea parent, s_battleEngine* engine, int subtitle, const char* pcmName)
{
    // Native formation adapters supply their authored subtitle index and PCM.
    const auto entry = engine->m3A8_overlayBattledata + engine->m3B0_subBattleId * 0x20;
    const auto table = readSaturnEA(entry + 0xC);
    const auto text = readSaturnEA(table + subtitle * 4);
    std::vector<std::uint8_t> pcm;
    if (!lagi::disc::read_file(pcmName, pcm) || pcm.empty())
    {
        lagi::platform::logging::writef("[BattleDialogue] missing %s\n", pcmName);
        return;
    }

    static const NanydDialogue::TypedTaskDefinition definition = {
        nullptr, &NanydDialogue::Update, nullptr, &NanydDialogue::Delete,
    };
    auto* task = createSubTask<NanydDialogue>(parent, &definition);
    if (!task)
        return;
    task->pcm = new std::vector<std::uint8_t>(std::move(pcm));
    task->displayed = true;
    setupVDP2StringRendering(3, 25, 38, 4);
    clearVdp2TextArea();
    VDP2DrawString(readSaturnString(text).c_str());
    lagi::platform::logging::writef("[BattleDialogue] start sub=%d text=%d file=%s\n", int(engine->m3B0_subBattleId), subtitle, pcmName);
}

void lagiStartNanydDialogue(p_workArea parent, s_battleEngine* engine)
{
    lagiStartBattleDialogue(parent, engine, 4, "B_A5_00.PCM");
}

} // namespace
