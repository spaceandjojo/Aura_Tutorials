// Copyright Time and Space


#include "Actor/AuraEffectActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

AAuraEffectActor::AAuraEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(FName("SceneRoot")));
}

void AAuraEffectActor::BeginPlay()
{
	Super::BeginPlay();
}

void AAuraEffectActor::ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	//获取目标Actor的ASC组件
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (TargetASC == nullptr) return;

	check(GameplayEffectClass);
	FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();//创建ASC组件Owner即目标Actor的GE上下文
	EffectContextHandle.AddSourceObject(this);//设置GE的来源，即AuraEffectActor
	FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(GameplayEffectClass, ActorLevel, EffectContextHandle);//创建GE的实例
	//将GE应用在Actor本身，即目标Actor，并将返回的FActiveGameplayEffectHandle储存（一旦应用了GE，这个GE就会被Active）
	const FActiveGameplayEffectHandle ActiveEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());

	//添加键值对，将类型为Infinite的GE和目标Actor的ASC联系起来，以便在应用时进行评估
	const bool bIsInfinite = EffectSpecHandle.Data.Get()->Def.Get()->DurationPolicy == EGameplayEffectDurationType::Infinite;
	if (bIsInfinite && InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	{
		ActiveEffectHandles.Add(ActiveEffectHandle, TargetASC);
	}
}

void AAuraEffectActor::OnOverlap(AActor* TargetActor)
{
	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
}

void AAuraEffectActor::OnEndOverlap(AActor* TargetActor)
{
	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
	//主动删除符合条件且持续类型为Infinite的GE
	if (InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!IsValid(TargetASC)) return;

		TArray<FActiveGameplayEffectHandle> HandlesToRemove;
		//循环遍历键值对，将键值对里储存的value和EndOverlap的Actor的ASC对比，如果一致则移除该GE，并将键值对的Key储存到数组HandlesToRemove中
		for (TTuple<FActiveGameplayEffectHandle, UAbilitySystemComponent*> HandlePair :ActiveEffectHandles)
		{
			if (TargetASC ==  HandlePair.Value)
			{
				TargetASC->RemoveActiveGameplayEffect(HandlePair.Key,1);
				HandlesToRemove.Add(HandlePair.Key);
			}
		}
		//将数组HandlesToRemove中的值删除
		for (FActiveGameplayEffectHandle& Handle : HandlesToRemove)
		{
			ActiveEffectHandles.FindAndRemoveChecked(Handle);
		}
	}
	
}



