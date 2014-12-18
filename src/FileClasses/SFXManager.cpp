/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <FileClasses/SFXManager.h>

#include <globals.h>

#include <FileClasses/FileManager.h>
#include <FileClasses/Vocfile.h>
#include <FileClasses/SaveWAV.h>

#include <FileClasses/adl/sound_adlib.h>

#include <misc/sound_util.h>


SFXManager::SFXManager() {
	// load voice and language specific sounds
	if(settings.general.language == "de") {
        loadNonEnglishVoice("G");
	} else if(settings.general.language == "fr") {
        loadNonEnglishVoice("F");
	} else {
        loadEnglishVoice();
	}

	for(int i = 0; i < NUM_SOUNDCHUNK; i++) {
		if(soundChunk[i] == NULL) {
			fprintf(stderr,"SFXManager::SFXManager: Not all sounds could be loaded\n");
			exit(EXIT_FAILURE);
		}
	}
}

SFXManager::~SFXManager() {
	// unload voice
	for(int i = 0; i < numLngVoice; i++) {
		if(lngVoice[i] != NULL) {
			Mix_FreeChunk(lngVoice[i]);
			lngVoice[i] = NULL;
		}
	}

	free(lngVoice);

	// unload sound
	for(int i = 0; i < NUM_SOUNDCHUNK; i++) {
		if(soundChunk[i] != NULL) {
			Mix_FreeChunk(soundChunk[i]);
			soundChunk[i] = NULL;
		}
	}
}

Mix_Chunk* SFXManager::getVoice(Voice_enum id, int house) {
    if(settings.general.language == "de" || settings.general.language == "fr") {
        return getNonEnglishVoice(id,house);
    } else {
		return getEnglishVoice(id,house);
	}
}

Mix_Chunk* SFXManager::getSound(Sound_enum id) {
	if(id >= NUM_SOUNDCHUNK)
		return NULL;

	return soundChunk[id];
}

Mix_Chunk* SFXManager::loadMixFromADL(std::string adlFile, int index) {

    SDL_RWops* rwop = pFileManager->openFile(adlFile);
    if(rwop == NULL) {
        fprintf(stderr,"SFXManager::LoadMixFromADL:Unable to load %s!\n",adlFile.c_str());
        return NULL;
    }

    SoundAdlibPC *pSoundAdlibPC = new SoundAdlibPC(rwop, false);
    Mix_Chunk* chunk = pSoundAdlibPC->getSubsong(index);
    delete pSoundAdlibPC;
    SDL_RWclose(rwop);

    return chunk;
}

void SFXManager::loadEnglishVoice() {
	numLngVoice = NUM_VOICE*NUM_HOUSES;

	if((lngVoice = (Mix_Chunk**) malloc(sizeof(Mix_Chunk*) * numLngVoice)) == NULL) {
		fprintf(stderr,"SFXManager::LoadVoice_English: Cannot allocate memory!\n");
		exit(EXIT_FAILURE);
	}

	for(int i = 0; i < numLngVoice; i++) {
		lngVoice[i] = NULL;
	}

	// now we can load
	for(int house = 0; house < NUM_HOUSES; house++) {
		Mix_Chunk* HouseNameChunk = NULL;

		std::string HouseString;
		int VoiceNum = house;
		switch(house) {
		    case HOUSE_HARKONNEN:
				HouseString = "H";
				HouseNameChunk = getChunkFromFile(HouseString + "HARK.VOC");
				break;
			case HOUSE_ATREIDES:
				HouseString = "A";
				HouseNameChunk = getChunkFromFile(HouseString + "ATRE.VOC");
				break;
			case HOUSE_ORDOS:
				HouseString = "O";
				HouseNameChunk = getChunkFromFile(HouseString + "ORDOS.VOC");
				break;
			case HOUSE_FREMEN:
				HouseString = "A";
				HouseNameChunk = getChunkFromFile(HouseString + "FREMEN.VOC");
				break;
			case HOUSE_SARDAUKAR:
				HouseString = "H";
				HouseNameChunk = getChunkFromFile(HouseString + "SARD.VOC");
				break;
			case HOUSE_MERCENARY:
				HouseString = "O";
				HouseNameChunk = getChunkFromFile(HouseString + "MERC.VOC");
				break;
		}

		// "... Harvester deployed"
		Mix_Chunk* Harvester = getChunkFromFile(HouseString + "HARVEST.VOC");
		Mix_Chunk* Deployed = getChunkFromFile(HouseString + "DEPLOY.VOC");
		lngVoice[HarvesterDeployed*NUM_HOUSES+VoiceNum] = concat3Chunks(HouseNameChunk, Harvester, Deployed);
		Mix_FreeChunk(Harvester);
		Mix_FreeChunk(Deployed);

		// "Contruction complete"
		lngVoice[ConstructionComplete*NUM_HOUSES+VoiceNum] = getChunkFromFile(HouseString + "CONST.VOC");

		// "Vehicle repaired"
		Mix_Chunk* Vehicle = getChunkFromFile(HouseString + "VEHICLE.VOC");
		Mix_Chunk* Repaired = getChunkFromFile(HouseString + "REPAIR.VOC");
		lngVoice[VehicleRepaired*NUM_HOUSES+VoiceNum] = concat2Chunks(Vehicle, Repaired);
		Mix_FreeChunk(Vehicle);
		Mix_FreeChunk(Repaired);

		// "Frigate has arrived"
		Mix_Chunk* FrigateChunk = getChunkFromFile(HouseString + "FRIGATE.VOC");
		Mix_Chunk* HasArrivedChunk = getChunkFromFile(HouseString + "ARRIVE.VOC");
		lngVoice[FrigateHasArrived*NUM_HOUSES+VoiceNum] = concat2Chunks(FrigateChunk, HasArrivedChunk);
		Mix_FreeChunk(FrigateChunk);
		Mix_FreeChunk(HasArrivedChunk);

		// "Your mission is complete"
		lngVoice[YourMissionIsComplete*NUM_HOUSES+VoiceNum] = getChunkFromFile(HouseString + "WIN.VOC");

		// "You have failed your mission"
		lngVoice[YouHaveFailedYourMission*NUM_HOUSES+VoiceNum] = getChunkFromFile(HouseString + "LOSE.VOC");

		// "Radar activated"/"Radar deactivated"
		Mix_Chunk* RadarChunk = getChunkFromFile(HouseString + "RADAR.VOC");
		Mix_Chunk* RadarActivatedChunk = getChunkFromFile(HouseString + "ON.VOC");
		Mix_Chunk* RadarDeactivatedChunk = getChunkFromFile(HouseString + "OFF.VOC");
		lngVoice[RadarActivated*NUM_HOUSES+VoiceNum] = concat2Chunks(RadarChunk, RadarActivatedChunk);
		lngVoice[RadarDeactivated*NUM_HOUSES+VoiceNum] = concat2Chunks(RadarChunk, RadarDeactivatedChunk);
		Mix_FreeChunk(RadarChunk);
		Mix_FreeChunk(RadarActivatedChunk);
		Mix_FreeChunk(RadarDeactivatedChunk);

		// "Bloom located"
		Mix_Chunk* Bloom = getChunkFromFile(HouseString + "BLOOM.VOC");
		Mix_Chunk* Located = getChunkFromFile(HouseString + "LOCATED.VOC");
		lngVoice[BloomLocated*NUM_HOUSES+VoiceNum] = concat2Chunks(Bloom, Located);
		Mix_FreeChunk(Bloom);
		Mix_FreeChunk(Located);

        // "Warning Wormsign"
        Mix_Chunk* WarningChunk = getChunkFromFile(HouseString + "WARNING.VOC");
        Mix_Chunk* WormSignChunk = getChunkFromFile(HouseString + "WORMY.VOC");
        lngVoice[WarningWormSign*NUM_HOUSES+VoiceNum] = concat2Chunks(WarningChunk, WormSignChunk);
        Mix_FreeChunk(WarningChunk);
        Mix_FreeChunk(WormSignChunk);

        // "Our base is under attack"
		lngVoice[BaseIsUnderAttack*NUM_HOUSES+VoiceNum] = getChunkFromFile(HouseString + "ATTACK.VOC");

        // "Saboteur approaching" and "Missile approaching"
        Mix_Chunk* SabotChunk = getChunkFromFile(HouseString + "SABOT.VOC");
        Mix_Chunk* MissileChunk = getChunkFromFile(HouseString + "MISSILE.VOC");
        Mix_Chunk* ApproachingChunk = getChunkFromFile(HouseString + "APPRCH.VOC");
        lngVoice[SaboteurApproaching*NUM_HOUSES+VoiceNum] = concat2Chunks(SabotChunk, ApproachingChunk);
        lngVoice[MissileApproaching*NUM_HOUSES+VoiceNum] = concat2Chunks(MissileChunk, ApproachingChunk);
        Mix_FreeChunk(SabotChunk);
        Mix_FreeChunk(MissileChunk);
        Mix_FreeChunk(ApproachingChunk);

		Mix_FreeChunk(HouseNameChunk);
	}

	for(int i = 0; i < numLngVoice; i++) {
		if(lngVoice[i] == NULL) {
			fprintf(stderr,"SFXManager::LoadVoice_English: Not all voice sounds could be loaded\n");
			exit(EXIT_FAILURE);
		}
	}

	// "Yes Sir"
    soundChunk[YesSir] = getChunkFromFile("ZREPORT1.VOC", "REPORT1.VOC");

	// "Reporting"
	soundChunk[Reporting] = getChunkFromFile("ZREPORT2.VOC", "REPORT2.VOC");

	// "Acknowledged"
	soundChunk[Acknowledged] = getChunkFromFile("ZREPORT3.VOC", "REPORT3.VOC");

	// "Affirmative"
	soundChunk[Affirmative] = getChunkFromFile("ZAFFIRM.VOC", "AFFIRM.VOC");

	// "Moving out"
	soundChunk[MovingOut] = getChunkFromFile("ZMOVEOUT.VOC", "MOVEOUT.VOC");

	// "Infantry out"
	soundChunk[InfantryOut] = getChunkFromFile("ZOVEROUT.VOC", "OVEROUT.VOC");

	// "Somthing's under the sand"
	soundChunk[SomethingUnderTheSand] = getChunkFromFile("SANDBUG.VOC");

	// "House Harkonnen"
	soundChunk[HouseHarkonnen] = getChunkFromFile("MHARK.VOC");

	// "House Atreides"
	soundChunk[HouseAtreides] = getChunkFromFile("MATRE.VOC");

	// "House Ordos"
	soundChunk[HouseOrdos] = getChunkFromFile("MORDOS.VOC");

	// Sfx
	soundChunk[PlaceStructure] = getChunkFromFile("EXDUD.VOC");
	soundChunk[ButtonClick] = getChunkFromFile("BUTTON.VOC");
	soundChunk[InvalidAction] = Mix_LoadWAV_RW(pFileManager->openFile("CANNOT.WAV"),1); // LoadMixFromADL("DUNE9.ADL", 47);
	soundChunk[CreditsTick] = Mix_LoadWAV_RW(pFileManager->openFile("CREDIT.WAV"),1);   // LoadMixFromADL("DUNE9.ADL", 38);
	soundChunk[Tick] = Mix_LoadWAV_RW(pFileManager->openFile("TICK.WAV"),1);
	soundChunk[RadarNoise] = getChunkFromFile("STATICP.VOC");
	soundChunk[Sound_ExplosionGas] = getChunkFromFile("EXGAS.VOC");
	soundChunk[Sound_ExplosionTiny] = getChunkFromFile("EXTINY.VOC");
	soundChunk[Sound_ExplosionSmall] = getChunkFromFile("EXSMALL.VOC");
	soundChunk[Sound_ExplosionMedium] = getChunkFromFile("EXMED.VOC");
	soundChunk[Sound_ExplosionLarge] = getChunkFromFile("EXLARGE.VOC");
	soundChunk[Sound_ExplosionStructure] = getChunkFromFile("CRUMBLE.VOC");
	soundChunk[Sound_WormAttack] = getChunkFromFile("WORMET3P.VOC");
	soundChunk[Sound_Gun] = getChunkFromFile("GUN.VOC");
	soundChunk[Sound_Rocket] = getChunkFromFile("ROCKET.VOC");
	soundChunk[Sound_Bloom] = getChunkFromFile("EXSAND.VOC");
	soundChunk[Sound_Scream1] = getChunkFromFile("VSCREAM1.VOC");
	soundChunk[Sound_Scream2] = getChunkFromFile("VSCREAM2.VOC");
	soundChunk[Sound_Scream3] = getChunkFromFile("VSCREAM3.VOC");
	soundChunk[Sound_Scream4] = getChunkFromFile("VSCREAM4.VOC");
	soundChunk[Sound_Scream5] = getChunkFromFile("VSCREAM5.VOC");
	soundChunk[Sound_Trumpet] = Mix_LoadWAV_RW(pFileManager->openFile("TRUMPET.WAV"),1);    // LoadMixFromADL("DUNE9.ADL", 30);
	soundChunk[Sound_Drop] = Mix_LoadWAV_RW(pFileManager->openFile("DROP.WAV"),1);          // LoadMixFromADL("DUNE9.ADL", 24);
	soundChunk[Sound_Squashed] = getChunkFromFile("SQUISH2.VOC");
	soundChunk[Sound_MachineGun] = getChunkFromFile("GUNMULTI.VOC");
	soundChunk[Sound_Sonic] = Mix_LoadWAV_RW(pFileManager->openFile("SONIC.WAV"),1);
	soundChunk[Sound_RocketSmall] = getChunkFromFile("MISLTINP.VOC");
}


Mix_Chunk* SFXManager::getEnglishVoice(Voice_enum id, int house) {
	if((int) id >= numLngVoice)
		return NULL;

	return lngVoice[id*NUM_HOUSES + house];
}

void SFXManager::loadNonEnglishVoice(std::string languagePrefix) {
	numLngVoice = NUM_VOICE;

	if((lngVoice = (Mix_Chunk**) malloc(sizeof(Mix_Chunk*) * NUM_VOICE)) == NULL) {
		fprintf(stderr,"SFXManager::LoadVoice_NonEnglish: Cannot allocate memory!\n");
		exit(EXIT_FAILURE);
	}

	for(int i = 0; i < NUM_VOICE; i++) {
		lngVoice[i] = NULL;
	}


	lngVoice[HarvesterDeployed] = getChunkFromFile(languagePrefix + "HARVEST.VOC");

	// "Contruction complete"
	lngVoice[ConstructionComplete] = getChunkFromFile(languagePrefix + "CONST.VOC");

	// "Vehicle repaired"
	lngVoice[VehicleRepaired] = getChunkFromFile(languagePrefix + "REPAIR.VOC");

	// "Frigate has arrived"
	lngVoice[FrigateHasArrived] = getChunkFromFile(languagePrefix + "FRIGATE.VOC");

	// "Your mission is complete" (No non-english voc available)
	lngVoice[YourMissionIsComplete] = createEmptyChunk();

	// "You have failed your mission" (No non-english voc available)
	lngVoice[YouHaveFailedYourMission] = createEmptyChunk();

	// "Radar activated"/"Radar deactivated"
	lngVoice[RadarActivated] = getChunkFromFile(languagePrefix + "ON.VOC");
	lngVoice[RadarDeactivated] = getChunkFromFile(languagePrefix + "OFF.VOC");

	// "Bloom located"
	lngVoice[BloomLocated] = getChunkFromFile(languagePrefix + "BLOOM.VOC");

	// "Warning Wormsign"
	if(pFileManager->exists(languagePrefix + "WORMY.VOC")) {
        Mix_Chunk* WarningChunk = getChunkFromFile(languagePrefix + "WARNING.VOC");
        Mix_Chunk* WormSignChunk = getChunkFromFile(languagePrefix + "WORMY.VOC");
        lngVoice[WarningWormSign] = concat2Chunks(WarningChunk, WormSignChunk);
        Mix_FreeChunk(WarningChunk);
        Mix_FreeChunk(WormSignChunk);
	} else {
        lngVoice[WarningWormSign] = getChunkFromFile(languagePrefix + "WARNING.VOC");
	}

    // "Our base is under attack"
	lngVoice[BaseIsUnderAttack] = getChunkFromFile(languagePrefix + "ATTACK.VOC");

    // "Saboteur approaching"
	lngVoice[SaboteurApproaching] = getChunkFromFile(languagePrefix + "SABOT.VOC");

    // "Missile approaching"
	lngVoice[MissileApproaching] = getChunkFromFile(languagePrefix + "MISSILE.VOC");

	for(int i = 0; i < numLngVoice; i++) {
		if(lngVoice[i] == NULL) {
			fprintf(stderr,"SFXManager::LoadVoice_NonEnglish: Not all voice sounds could be loaded\n");
			exit(EXIT_FAILURE);
		}
	}

	// "Yes Sir"
	soundChunk[YesSir] = getChunkFromFile(languagePrefix + "REPORT1.VOC");

	// "Reporting"
	soundChunk[Reporting] = getChunkFromFile(languagePrefix + "REPORT2.VOC");

	// "Acknowledged"
	soundChunk[Acknowledged] = getChunkFromFile(languagePrefix + "REPORT3.VOC");

	// "Affirmative"
	soundChunk[Affirmative] = getChunkFromFile(languagePrefix + "AFFIRM.VOC");

	// "Moving out"
	soundChunk[MovingOut] = getChunkFromFile(languagePrefix + "MOVEOUT.VOC");

	// "Infantry out"
	soundChunk[InfantryOut] = getChunkFromFile(languagePrefix + "OVEROUT.VOC");

	// "Somthing's under the sand"
	soundChunk[SomethingUnderTheSand] = getChunkFromFile("SANDBUG.VOC");

	// "House Atreides"
	soundChunk[HouseAtreides] = getChunkFromFile(languagePrefix + "ATRE.VOC");

	// "House Ordos"
	soundChunk[HouseOrdos] = getChunkFromFile(languagePrefix + "ORDOS.VOC");

	// "House Harkonnen"
	soundChunk[HouseHarkonnen] = getChunkFromFile(languagePrefix + "HARK.VOC");

	// Sfx
	soundChunk[PlaceStructure] = getChunkFromFile("EXDUD.VOC");
	soundChunk[ButtonClick] = getChunkFromFile("BUTTON.VOC");
	soundChunk[InvalidAction] = Mix_LoadWAV_RW(pFileManager->openFile("CANNOT.WAV"),1); // LoadMixFromADL("DUNE9.ADL", 47);
	soundChunk[CreditsTick] = Mix_LoadWAV_RW(pFileManager->openFile("CREDIT.WAV"),1);   // LoadMixFromADL("DUNE9.ADL", 38);
	soundChunk[Tick] = Mix_LoadWAV_RW(pFileManager->openFile("TICK.WAV"),1);
	soundChunk[RadarNoise] = getChunkFromFile("STATICP.VOC");
	soundChunk[Sound_ExplosionGas] = getChunkFromFile("EXGAS.VOC");
	soundChunk[Sound_ExplosionTiny] = getChunkFromFile("EXTINY.VOC");
	soundChunk[Sound_ExplosionSmall] = getChunkFromFile("EXSMALL.VOC");
	soundChunk[Sound_ExplosionMedium] = getChunkFromFile("EXMED.VOC");
	soundChunk[Sound_ExplosionLarge] = getChunkFromFile("EXLARGE.VOC");
	soundChunk[Sound_ExplosionStructure] = getChunkFromFile("CRUMBLE.VOC");
	soundChunk[Sound_WormAttack] = getChunkFromFile("WORMET3P.VOC");
	soundChunk[Sound_Gun] = getChunkFromFile("GUN.VOC");
	soundChunk[Sound_Rocket] = getChunkFromFile("ROCKET.VOC");
	soundChunk[Sound_Bloom] = getChunkFromFile("EXSAND.VOC");
	soundChunk[Sound_Scream1] = getChunkFromFile("VSCREAM1.VOC");
	soundChunk[Sound_Scream2] = getChunkFromFile("VSCREAM2.VOC");
	soundChunk[Sound_Scream3] = getChunkFromFile("VSCREAM3.VOC");
	soundChunk[Sound_Scream4] = getChunkFromFile("VSCREAM4.VOC");
	soundChunk[Sound_Scream5] = getChunkFromFile("VSCREAM5.VOC");
	soundChunk[Sound_Trumpet] = Mix_LoadWAV_RW(pFileManager->openFile("TRUMPET.WAV"),1);    // LoadMixFromADL("DUNE9.ADL", 30);
	soundChunk[Sound_Drop] = Mix_LoadWAV_RW(pFileManager->openFile("DROP.WAV"),1);          // LoadMixFromADL("DUNE9.ADL", 24);
	soundChunk[Sound_Squashed] = getChunkFromFile("SQUISH2.VOC");
	soundChunk[Sound_MachineGun] = getChunkFromFile("GUNMULTI.VOC");
	soundChunk[Sound_Sonic] = Mix_LoadWAV_RW(pFileManager->openFile("SONIC.WAV"),1);
	soundChunk[Sound_RocketSmall] = getChunkFromFile("MISLTINP.VOC");
}

Mix_Chunk* SFXManager::getNonEnglishVoice(Voice_enum id, int house) {
	if((int)id >= numLngVoice)
		return NULL;

	return lngVoice[id];
}
