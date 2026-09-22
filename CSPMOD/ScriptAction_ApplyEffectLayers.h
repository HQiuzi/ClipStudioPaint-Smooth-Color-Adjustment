#pragma once

class ScriptAction_ApplyEffectLayers
{
public:
    enum class Result
    {
        Success,
        Unavailable,
        InvalidSelection,
        NoTargetLayers,
        OperationFailed
    };

    static bool Available();
    static Result Run();
};
