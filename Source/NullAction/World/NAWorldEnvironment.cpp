#include "World/NAWorldEnvironment.h"

#include "NullAction.h"
#include "Character/NACharacterBase.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "World/NAPrecipitation.h"
#include "World/NATestArena.h"

namespace NAEnvironment
{
	constexpr float SunLux = 10.f;
	constexpr float MoonLux = 0.25f;
}

ANAWorldEnvironment::ANAWorldEnvironment()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(2.f); // Time and weather change slowly; clients extrapolate in between.

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(SceneRoot);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetAtmosphereSunLight(true);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetIntensity(NAEnvironment::SunLux);

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(SceneRoot);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->SetAtmosphereSunLight(true);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetIntensity(NAEnvironment::MoonLux);
	Moon->SetLightColor(FLinearColor(0.55f, 0.65f, 1.f));
	Moon->SetCastShadows(true);

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(SceneRoot);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(SceneRoot);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(SceneRoot);
	Fog->SetVolumetricFog(true); // Light shafts through the trees and mist at dawn.
	Fog->SetFogHeightFalloff(0.06f);

	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(SceneRoot);
	Clouds->SetLayerBottomAltitude(4.f);  // km
	Clouds->SetLayerHeight(6.f);

	Grade = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Grade"));
	Grade->SetupAttachment(SceneRoot);
	Grade->bUnbound = true;
	Grade->Priority = -1.f; // Any post-process volume placed in a level wins.
	FPostProcessSettings& Look = Grade->Settings;
	Look.bOverride_ColorSaturation = true;
	Look.ColorSaturation = FVector4(0.9f, 0.9f, 0.9f, 1.f);
	Look.bOverride_ColorContrast = true;
	Look.ColorContrast = FVector4(1.08f, 1.08f, 1.08f, 1.f);
	Look.bOverride_ColorGain = true;
	Look.ColorGain = FVector4(1.03f, 1.f, 0.96f, 1.f); // Slightly warm.
	Look.bOverride_VignetteIntensity = true;
	Look.VignetteIntensity = 0.45f;
	Look.bOverride_BloomIntensity = true;
	Look.BloomIntensity = 0.55f;
	Look.bOverride_FilmGrainIntensity = true;
	Look.FilmGrainIntensity = 0.08f;
	Look.bOverride_AutoExposureBias = true;
	Look.AutoExposureBias = 0.3f;
	Look.bOverride_AmbientOcclusionIntensity = true;
	Look.AmbientOcclusionIntensity = 0.65f;
}

ANAWorldEnvironment* ANAWorldEnvironment::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<ANAWorldEnvironment> It(const_cast<UWorld*>(World));
	return It ? *It : nullptr;
}

void ANAWorldEnvironment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAWorldEnvironment, ReplicatedTimeOfDay);
	DOREPLIFETIME(ANAWorldEnvironment, HoursPerSecond);
	DOREPLIFETIME(ANAWorldEnvironment, FromWeather);
	DOREPLIFETIME(ANAWorldEnvironment, ToWeather);
	DOREPLIFETIME(ANAWorldEnvironment, TransitionStartServerTime);
	DOREPLIFETIME(ANAWorldEnvironment, TransitionDuration);
}

void ANAWorldEnvironment::BeginPlay()
{
	Super::BeginPlay();

	// The world environment owns lighting: switch off the test arena's simple sun.
	for (TActorIterator<ANATestArena> It(GetWorld()); It; ++It)
	{
		It->RemoveBuiltInLighting();
	}

	if (HasAuthority())
	{
		HoursPerSecond = 24.f / (FMath::Max(DayLengthMinutes, 1.f) * 60.f);
		LocalTimeOfDay = ReplicatedTimeOfDay = StartTimeOfDay;
		TransitionStartServerTime = GetServerTime() - 1.f;
		ScheduleNextWeather();
	}
	else
	{
		LocalTimeOfDay = ReplicatedTimeOfDay;
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Precipitation = GetWorld()->SpawnActor<ANAPrecipitation>(ANAPrecipitation::StaticClass(), FTransform::Identity, Params);
	}
	UpdateVisuals(0.f);
}

float ANAWorldEnvironment::GetServerTime() const
{
	const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return State ? State->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
}

float ANAWorldEnvironment::GetTransitionAlpha() const
{
	return FMath::Clamp((GetServerTime() - TransitionStartServerTime) / FMath::Max(TransitionDuration, 0.1f), 0.f, 1.f);
}

// ----- Queries ------------------------------------------------------------------------------------

float ANAWorldEnvironment::GetDaylight() const
{
	// Sun height: 0 at 06:00 and 18:00, 1 at noon, below zero at night.
	const float Elevation = FMath::Sin((LocalTimeOfDay - 6.f) / 12.f * PI);
	return FMath::Clamp(Elevation * 2.5f, 0.f, 1.f);
}

bool ANAWorldEnvironment::IsNight() const
{
	return LocalTimeOfDay < 5.5f || LocalTimeOfDay > 19.5f;
}

ENAWeather ANAWorldEnvironment::GetWeather() const
{
	return GetTransitionAlpha() >= 0.5f ? ToWeather : FromWeather;
}

FNAWeatherProfile ANAWorldEnvironment::GetBlendedProfile() const
{
	const FNAWeatherProfile A = GetProfile(FromWeather);
	const FNAWeatherProfile B = GetProfile(ToWeather);
	const float T = GetTransitionAlpha();
	FNAWeatherProfile P;
	P.FogDensity = FMath::Lerp(A.FogDensity, B.FogDensity, T);
	P.SunScale = FMath::Lerp(A.SunScale, B.SunScale, T);
	P.SkyScale = FMath::Lerp(A.SkyScale, B.SkyScale, T);
	P.Rain = FMath::Lerp(A.Rain, B.Rain, T);
	P.Snow = FMath::Lerp(A.Snow, B.Snow, T);
	P.Dust = FMath::Lerp(A.Dust, B.Dust, T);
	P.Visibility = FMath::Lerp(A.Visibility, B.Visibility, T);
	P.TemperatureOffset = FMath::Lerp(A.TemperatureOffset, B.TemperatureOffset, T);
	P.MoveSpeed = FMath::Lerp(A.MoveSpeed, B.MoveSpeed, T);
	P.NoiseRange = FMath::Lerp(A.NoiseRange, B.NoiseRange, T);
	P.LightningPerMinute = FMath::Lerp(A.LightningPerMinute, B.LightningPerMinute, T);
	P.Tint = FMath::Lerp(A.Tint, B.Tint, T);
	return P;
}

float ANAWorldEnvironment::GetVisibilityScale() const
{
	// Darkness and weather both cut how far you can be seen.
	const float NightFactor = FMath::Lerp(0.55f, 1.f, GetDaylight());
	return GetBlendedProfile().Visibility * NightFactor;
}

float ANAWorldEnvironment::GetAmbientTemperature() const
{
	// 12 °C before dawn up to 24 °C mid-afternoon, plus the weather.
	const float DayCurve = FMath::Sin((LocalTimeOfDay - 9.f) / 24.f * 2.f * PI) * 0.5f + 0.5f;
	return FMath::Lerp(12.f, 24.f, DayCurve) + GetBlendedProfile().TemperatureOffset;
}

FNAWeatherProfile ANAWorldEnvironment::GetProfile(ENAWeather Weather)
{
	FNAWeatherProfile P;
	switch (Weather)
	{
	case ENAWeather::Clear:
		break;
	case ENAWeather::Cloudy:
		P.FogDensity = 0.02f; P.SunScale = 0.35f; P.SkyScale = 0.8f; P.Visibility = 0.95f; P.TemperatureOffset = -2.f;
		break;
	case ENAWeather::Rain:
		P.FogDensity = 0.035f; P.SunScale = 0.2f; P.SkyScale = 0.6f; P.Rain = 0.5f; P.Visibility = 0.8f; P.TemperatureOffset = -5.f; P.NoiseRange = 0.75f;
		break;
	case ENAWeather::HeavyRain:
		P.FogDensity = 0.06f; P.SunScale = 0.1f; P.SkyScale = 0.45f; P.Rain = 1.f; P.Visibility = 0.6f; P.TemperatureOffset = -7.f; P.NoiseRange = 0.55f;
		break;
	case ENAWeather::Thunderstorm:
		P.FogDensity = 0.07f; P.SunScale = 0.05f; P.SkyScale = 0.3f; P.Rain = 1.f; P.Visibility = 0.55f; P.TemperatureOffset = -8.f; P.NoiseRange = 0.5f; P.LightningPerMinute = 6.f;
		break;
	case ENAWeather::Fog:
		P.FogDensity = 0.3f; P.SunScale = 0.3f; P.SkyScale = 0.7f; P.Visibility = 0.35f; P.TemperatureOffset = -3.f; P.NoiseRange = 0.9f;
		break;
	case ENAWeather::Snow:
		P.FogDensity = 0.05f; P.SunScale = 0.3f; P.SkyScale = 0.8f; P.Snow = 0.6f; P.Visibility = 0.75f; P.TemperatureOffset = -18.f; P.MoveSpeed = 0.88f; P.NoiseRange = 0.8f;
		break;
	case ENAWeather::Blizzard:
		P.FogDensity = 0.25f; P.SunScale = 0.08f; P.SkyScale = 0.5f; P.Snow = 1.f; P.Visibility = 0.3f; P.TemperatureOffset = -28.f; P.MoveSpeed = 0.75f; P.NoiseRange = 0.5f;
		break;
	case ENAWeather::Windy:
		P.FogDensity = 0.015f; P.SunScale = 0.8f; P.Visibility = 0.95f; P.TemperatureOffset = -4.f; P.NoiseRange = 0.7f;
		break;
	case ENAWeather::DustStorm:
		P.FogDensity = 0.2f; P.SunScale = 0.25f; P.SkyScale = 0.6f; P.Dust = 1.f; P.Visibility = 0.35f; P.TemperatureOffset = 6.f; P.NoiseRange = 0.6f;
		P.Tint = FLinearColor(0.55f, 0.38f, 0.15f, 0.25f);
		break;
	}
	return P;
}

const TCHAR* ANAWorldEnvironment::WeatherName(ENAWeather Weather)
{
	switch (Weather)
	{
	case ENAWeather::Clear:        return TEXT("Clear");
	case ENAWeather::Cloudy:       return TEXT("Cloudy");
	case ENAWeather::Rain:         return TEXT("Rain");
	case ENAWeather::HeavyRain:    return TEXT("HeavyRain");
	case ENAWeather::Thunderstorm: return TEXT("Thunderstorm");
	case ENAWeather::Fog:          return TEXT("Fog");
	case ENAWeather::Snow:         return TEXT("Snow");
	case ENAWeather::Blizzard:     return TEXT("Blizzard");
	case ENAWeather::Windy:        return TEXT("Windy");
	case ENAWeather::DustStorm:    return TEXT("DustStorm");
	}
	return TEXT("?");
}

bool ANAWorldEnvironment::ParseWeather(const FString& Text, ENAWeather& OutWeather)
{
	for (uint8 Value = 0; Value <= static_cast<uint8>(ENAWeather::DustStorm); ++Value)
	{
		const ENAWeather Candidate = static_cast<ENAWeather>(Value);
		if (Text.Equals(WeatherName(Candidate), ESearchCase::IgnoreCase))
		{
			OutWeather = Candidate;
			return true;
		}
	}
	return false;
}

// ----- Server control ----------------------------------------------------------------------------

void ANAWorldEnvironment::SetWeather(ENAWeather NewWeather, float TransitionSeconds)
{
	if (!HasAuthority())
	{
		return;
	}
	FromWeather = GetWeather();
	ToWeather = NewWeather;
	TransitionStartServerTime = GetServerTime();
	TransitionDuration = FMath::Max(TransitionSeconds, 0.1f);
	ScheduleNextWeather();
	NotifyWeatherChanged();
}

void ANAWorldEnvironment::SetTimeOfDay(float Hours)
{
	if (HasAuthority())
	{
		LocalTimeOfDay = ReplicatedTimeOfDay = FMath::Fmod(FMath::Max(Hours, 0.f), 24.f);
	}
}

void ANAWorldEnvironment::SetDayLengthMinutes(float Minutes)
{
	if (HasAuthority())
	{
		HoursPerSecond = 24.f / (FMath::Clamp(Minutes, 1.f, 1440.f) * 60.f);
	}
}

void ANAWorldEnvironment::ScheduleNextWeather()
{
	NextWeatherChangeTime = GetServerTime() + FMath::FRandRange(WeatherDurationMinutes.X, WeatherDurationMinutes.Y) * 60.f;
}

ENAWeather ANAWorldEnvironment::PickNextWeather() const
{
	// Natural progressions: clear skies cloud over, clouds bring rain, storms clear up, etc.
	// (Different regions will get their own tables in the open world.)
	const ENAWeather Now = ToWeather;
	auto Pick = [](std::initializer_list<ENAWeather> Options)
	{
		const int32 Index = FMath::RandRange(0, static_cast<int32>(Options.size()) - 1);
		return *(Options.begin() + Index);
	};
	switch (Now)
	{
	case ENAWeather::Clear:        return Pick({ ENAWeather::Clear, ENAWeather::Cloudy, ENAWeather::Cloudy, ENAWeather::Windy, ENAWeather::Fog });
	case ENAWeather::Cloudy:       return Pick({ ENAWeather::Clear, ENAWeather::Rain, ENAWeather::Rain, ENAWeather::Snow, ENAWeather::Windy });
	case ENAWeather::Rain:         return Pick({ ENAWeather::Cloudy, ENAWeather::HeavyRain, ENAWeather::Thunderstorm, ENAWeather::Clear });
	case ENAWeather::HeavyRain:    return Pick({ ENAWeather::Rain, ENAWeather::Thunderstorm, ENAWeather::Cloudy });
	case ENAWeather::Thunderstorm: return Pick({ ENAWeather::HeavyRain, ENAWeather::Rain, ENAWeather::Cloudy });
	case ENAWeather::Fog:          return Pick({ ENAWeather::Clear, ENAWeather::Cloudy });
	case ENAWeather::Snow:         return Pick({ ENAWeather::Cloudy, ENAWeather::Blizzard, ENAWeather::Snow, ENAWeather::Clear });
	case ENAWeather::Blizzard:     return Pick({ ENAWeather::Snow, ENAWeather::Cloudy });
	case ENAWeather::Windy:        return Pick({ ENAWeather::Clear, ENAWeather::DustStorm, ENAWeather::Cloudy });
	case ENAWeather::DustStorm:    return Pick({ ENAWeather::Windy, ENAWeather::Clear });
	}
	return ENAWeather::Clear;
}

void ANAWorldEnvironment::NotifyWeatherChanged()
{
	// Snow slows walking: every character refreshes its speed (runs on server and clients).
	for (TActorIterator<ANACharacterBase> It(GetWorld()); It; ++It)
	{
		It->RefreshMovementSpeed();
	}
}

void ANAWorldEnvironment::OnRep_TimeOfDay()
{
	LocalTimeOfDay = ReplicatedTimeOfDay;
}

void ANAWorldEnvironment::OnRep_Weather()
{
	NotifyWeatherChanged();
}

void ANAWorldEnvironment::MulticastLightning_Implementation()
{
	if (!GetWorld())
	{
		return;
	}
	LightningFlashUntil = GetWorld()->GetTimeSeconds() + 0.18f;
	if (ThunderSound)
	{
		UGameplayStatics::PlaySound2D(this, ThunderSound, 1.f, FMath::FRandRange(0.85f, 1.1f));
	}
}

// ----- Tick ---------------------------------------------------------------------------------------

void ANAWorldEnvironment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	LocalTimeOfDay = FMath::Fmod(LocalTimeOfDay + HoursPerSecond * DeltaSeconds, 24.f);

	if (HasAuthority())
	{
		ReplicatedTimeOfDay = LocalTimeOfDay;

		const float ServerNow = GetServerTime();
		if (ServerNow >= NextWeatherChangeTime)
		{
			SetWeather(PickNextWeather(), 45.f);
		}

		const float LightningRate = GetBlendedProfile().LightningPerMinute;
		if (LightningRate > 0.f && FMath::FRand() < LightningRate / 60.f * DeltaSeconds)
		{
			MulticastLightning();
		}

		// While a transition runs, speeds change gradually; refresh a few times a second.
		if (GetTransitionAlpha() < 1.f && FMath::FRand() < DeltaSeconds * 2.f)
		{
			NotifyWeatherChanged();
		}
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		UpdateVisuals(DeltaSeconds);
	}
}

void ANAWorldEnvironment::UpdateVisuals(float DeltaSeconds)
{
	const FNAWeatherProfile Profile = GetBlendedProfile();
	const float Daylight = GetDaylight();

	// Sun travels east -> overhead -> west; the moon sits opposite.
	const float SunPitch = -(LocalTimeOfDay - 6.f) * 15.f;
	Sun->SetWorldRotation(FRotator(SunPitch, 35.f, 0.f));
	Moon->SetWorldRotation(FRotator(SunPitch + 180.f, 35.f, 0.f));

	const bool bFlash = GetWorld() && GetWorld()->GetTimeSeconds() < LightningFlashUntil;
	Sun->SetIntensity(NAEnvironment::SunLux * Daylight * Profile.SunScale + (bFlash ? 25.f : 0.f));
	Moon->SetIntensity(NAEnvironment::MoonLux * (1.f - Daylight) * FMath::Max(Profile.SunScale, 0.3f));
	SkyLight->SetIntensity(FMath::Lerp(0.06f, 1.f, Daylight) * Profile.SkyScale + (bFlash ? 3.f : 0.f));
	Fog->SetFogDensity(Profile.FogDensity);

	if (Precipitation)
	{
		Precipitation->SetIntensity(Profile.Rain, Profile.Snow, Profile.Dust);
	}
}
