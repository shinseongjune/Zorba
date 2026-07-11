import sys

import unreal


MOVES = (
    ("/Game/StarterContent", "/Game/90_ExternalAssets/Marketplace/StarterContent"),
    (
        "/Game/Polyart/ModularStylizedChars/Textures",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Textures",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Materials",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Materials",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Meshes",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Meshes",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Animations",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Animations",
    ),
    (
        "/Game/Polyart/SharedResources/Characters/Textures",
        "/Game/90_ExternalAssets/Marketplace/Polyart/SharedResources/Characters/Textures",
    ),
    (
        "/Game/Polyart/SharedResources/Characters/Materials",
        "/Game/90_ExternalAssets/Marketplace/Polyart/SharedResources/Characters/Materials",
    ),
    (
        "/Game/Polyart/SharedResources/Characters/Meshes",
        "/Game/90_ExternalAssets/Marketplace/Polyart/SharedResources/Characters/Meshes",
    ),
    (
        "/Game/Polyart/SharedResources/Characters/AnimationStuff",
        "/Game/90_ExternalAssets/Marketplace/Polyart/SharedResources/Characters/AnimationStuff",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Data/BodyPaint/BodyPaintParts",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Data/BodyPaint/BodyPaintParts",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Data/ColorSets",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Data/ColorSets",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Data/CustomizationWidgetInfo",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Data/CustomizationWidgetInfo",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Data/DataHolders",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Data/DataHolders",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Data/IndividualParts",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Data/IndividualParts",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Data/Skins",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Data/Skins",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Data/SkinSetPairs",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Data/SkinSetPairs",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Blueprints",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Blueprints",
    ),
    (
        "/Game/Polyart/ModularStylizedChars/Levels",
        "/Game/90_ExternalAssets/Marketplace/Polyart/ModularStylizedChars/Levels",
    ),
    (
        "/Game/Polyart/SharedResources/Characters/DataAssets",
        "/Game/90_ExternalAssets/Marketplace/Polyart/SharedResources/Characters/DataAssets",
    ),
    (
        "/Game/Polyart/SharedResources/Characters/Blueprints",
        "/Game/90_ExternalAssets/Marketplace/Polyart/SharedResources/Characters/Blueprints",
    ),
)


def ensure_parent_directory(path):
    parent = path.rsplit("/", 1)[0]
    if not unreal.EditorAssetLibrary.does_directory_exist(parent):
        if not unreal.EditorAssetLibrary.make_directory(parent):
            raise RuntimeError(f"Failed to create destination parent: {parent}")


def move_directory(source, destination):
    if not unreal.EditorAssetLibrary.does_directory_exist(source):
        unreal.log(f"Skip missing source directory: {source}")
        return False

    if unreal.EditorAssetLibrary.does_directory_exist(destination):
        unreal.log(f"Skip existing destination directory: {destination}")
        return False

    ensure_parent_directory(destination)

    unreal.log(f"Moving {source} -> {destination}")
    if not unreal.EditorAssetLibrary.rename_directory(source, destination):
        raise RuntimeError(f"Failed to move {source} -> {destination}")

    return True


def main():
    moved_any = False
    errors = []

    for source, destination in MOVES:
        try:
            moved_any = move_directory(source, destination) or moved_any
        except Exception as exc:
            errors.append(f"{source} -> {destination}: {exc}")
            unreal.log_error(errors[-1])

    if moved_any:
        unreal.EditorAssetLibrary.save_directory(
            "/Game/90_ExternalAssets/Marketplace",
            only_if_is_dirty=False,
            recursive=True,
        )
        unreal.EditorAssetLibrary.save_directory(
            "/Game",
            only_if_is_dirty=True,
            recursive=True,
        )
    else:
        unreal.log("No external asset folders needed moving.")

    if errors:
        raise RuntimeError("Some asset folders could not be moved. See log for details.")


try:
    main()
except Exception as exc:
    unreal.log_error(str(exc))
    sys.exit(1)
