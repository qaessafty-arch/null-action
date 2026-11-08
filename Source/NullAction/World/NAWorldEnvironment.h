#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NAWorldEnvironment.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UVolumetricCloudComponent;
class UPostProcessComponent;
class ANAPrecipitation;
class USoundBase;

UENUM(BlueprintType)
enum class ENAWeather : uint8
{
	Clear,
	Cloudy,
	Rain,
	HeavyRain,
	Thunderstorm,
	Fog,
	Snow,
	Blizzard,
	Windy,
	DustStorm
};

/** How a weather type looks and what it does to gameplay. Blended during transitions. */
struct FNAWeatherProfile
{
	float FogDensity = 0.01f;
	float SunScale = 1.f;          // Direct sunlight strength.
	float SkyScale = 1.f;          // Ambient sky light strength.
	float Rain = 0.f;              // 0..1 rain streak density.
	float Snow = 0.f;              // 0..1 snowflake density.
	float Dust = 0.f;              // 0..1 blowing dust.
	float Visibility = 1.f;        // Multiplies how far AI can see.
	float TemperatureOffset = 0.f; // Added to ambient °C.
	float MoveSpeed = 1.f;         // Multiplies walking speed (deep snow).
	float NoiseRange = 1.f;        // Multiplies how far footsteps carry (rain masks them).
	float LightningPerMinute = 0.f;
	FLinearColor Tint = FLinearColor::Transparent; // Screen tint (dust).
};

/**
 * One per world, spawned by the server. Owns the sun, moon, sky, fog and the replicated
 * time of day and weather, so every player sees the same conditions. Weather changes on its own
 * (server-side schedule) and blends smoothly; the owner can force it with NAWeather / NATime.
 *
 * Gameplay hooks other systems read: ambient temperature, AI visibility, movement speed,
 * footstep noise range, night (creatures hunt more aggressively).
 */
UCLASS(NotPlaceable)
class NULLACTION_API ANAWorldEnvironment : public AActor
{
	GENERATED_BODY()

public:
	ANAWorldEnvironment();

	static ANAWorldEnvironment* Get(const UObject* WorldContext);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	float GetTimeOfDay() const { return LocalTimeOfDay; }
	bool IsNight() const;
	float GetDaylight() const; // 0 at night .. 1 at midday
	ENAWeather GetWeather() const;
	FNAWeatherProfile GetBlendedProfile() const;
	float GetVisibilityScale() const;
	float GetAmbientTemperature() const;
	float GetMoveSpeedScale() const { return GetBlendedProfile().MoveSpeed; }
	float GetNoiseScale() const { return GetBlendedProfile().NoiseRange; }

	/** Server only. */
	void SetWeather(ENAWeather NewWeather, float TransitionSeconds = 30.f);
	void SetTimeOfDay(float Hours);
	void SetDayLengthMinutes(float Minutes);

	static const TCHAR* WeatherName(ENAWeather Weather);
	static bool ParseWeather(const FString& Text, ENAWeather& OutWeather);
	static FNAWeatherProfile GetProfile(ENAWeather Weather);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Environment") TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Environment") TObjectPtr<UDirectionalLightComponent> Moon;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Environment") TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Environment") TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Environment") TObjectPtr<UExponentialHeightFogComponent> Fog;
	/** Engine-provided volumetric clouds (default engine cloud material, no extra assets). */
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Environment") TObjectPtr<UVolumetricCloudComponent> Clouds;
	/** World-wide colour grade: the ashen NULL-ACTION look (slight desaturation, warm highlights, vignette). */
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Environment") TObjectPtr<UPostProcessComponent> Grade;

	/** Optional thunder sound, played after lightning if assigned. */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Environment") TObjectPtr<USoundBase> ThunderSound;

	/** Real minutes for a full 24-hour day. */
	UPROPERTY(EditAnywhere, Category = "NullAction|Environment") float DayLengthMinutes = 40.f;
	UPROPERTY(EditAnywhere, Category = "NullAction|Environment") float StartTimeOfDay = 9.f;
	/** How long each weather lasts before the server picks another (random within this range). */
	UPROPERTY(EditAnywhere, Category = "NullAction|Environment") FVector2D WeatherDurationMinutes = FVector2D(4.f, 9.f);

	UPROPERTY(ReplicatedUsing = OnRep_TimeOfDay) float ReplicatedTimeOfDay = 9.f;
	UPROPERTY(Replicated) float HoursPerSecond = 24.f / (40.f * 60.f);
	UPROPERTY(ReplicatedUsing = OnRep_Weather) ENAWeather FromWeather = ENAWeather::Clear;
	UPROPERTY(ReplicatedUsing = OnRep_Weather) ENAWeather ToWeather = ENAWeather::Clear;
	UPROPERTY(Replicated) float TransitionStartServerTime = 0.f;
	UPROPERTY(Replicated) float TransitionDuration = 1.f;

	UFUNCTION() void OnRep_TimeOfDay();
	UFUNCTION() void OnRep_Weather();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastLightning();

private:
	float GetServerTime() const;
	float GetTransitionAlpha() const;
	void UpdateVisuals(float DeltaSeconds);
	void ScheduleNextWeather();
	void NotifyWeatherChanged();
	ENAWeather PickNextWeather() const;

	UPROPERTY(Transient) TObjectPtr<ANAPrecipitation> Precipitation;

	float LocalTimeOfDay = 9.f;
	float NextWeatherChangeTime = 0.f;
	float LightningFlashUntil = -100.f;
	bool bLastWasHeavy = false;
};
