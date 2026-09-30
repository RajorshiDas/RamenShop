# Quick Reference: Material & Lighting System

## Using PBR Materials in Your Code

### Example 1: Apply Material to Food Object
```cpp
// Ceramic bowl - matte glaze
setMaterialPBR(Materials::Ceramic, BOWL_RED);
drawBowl({0, 0, 0});

// Glossy broth surface
setMaterialPBR(Materials::Broth, BROTH);
setMaterialGloss(0.90f, 0.75f, 0.50f, 85.0f);  // Blinn-Phong values
drawCylinder({0, 0.29f, 0}, NO_ROT, {0.83f, 0.02f, 0.83f}, BROTH);
```

### Example 2: Apply Material to Metal Object
```cpp
// Polished stainless steel kettle
setMaterialPBRMetallic(Materials::Polished, STEEL);
beginSphereReflect();  // Enable environment mapping
drawCylinder({0, 0, 0}, NO_ROT, {0.5f, 0.45f, 0.5f}, STEEL);
endSphereReflect();
```

### Example 3: Wood Material
```cpp
// Polished wood counter top
setMaterialPBR(Materials::WoodPolished, WOOD);
drawCuboid({0, 1.0f, 0.05f}, NO_ROT, {6.2f, 0.06f, 0.8f}, WOOD);
```

### Example 4: Fabric Material
```cpp
// Cushion material
setMaterialPBR(Materials::Fabric, CUSHION);
drawCylinder({0, 0.72f, 0}, NO_ROT, {0.38f, 0.06f, 0.38f}, CUSHION);
```

## Material Presets Available

### Dielectric (Non-Metal)
- `Materials::Ceramic` - Pottery with matte glaze
- `Materials::Porcelain` - Fine china, very smooth
- `Materials::GlazedMatte` - Matte glazed finish
- `Materials::Noodle` - Starchy starch surface
- `Materials::Broth` - Smooth liquid
- `Materials::EggYolk` - Slightly glossy egg
- `Materials::SeaweedNori` - Matte seaweed
- `Materials::WoodPolished` - Satin wood finish
- `Materials::WoodMatte` - Raw wood
- `Materials::Fabric` - Cloth/textile
- `Materials::Plastic` - Modern plastic
- `Materials::ClearGlass` - Window glass
- `Materials::FrostedGlass` - Frosted surface
- `Materials::Glass` - Drinking glass

### Metallic
- `Materials::Polished` - Chrome/mirror polish
- `Materials::BrushedMetal` - Brushed steel
- `Materials::Copper` - Shiny copper
- `Materials::Gold` - Polished gold

## Roughness & Shininess Reference

| Roughness | Appearance | Shininess | GL_SHININESS |
|-----------|-----------|-----------|--------|
| 0.05 | Mirror, very sharp | 120-128 | 120+ |
| 0.1 | Polished metal | 100-115 | 100+ |
| 0.15 | Porcelain/fine china | 80-90 | 80+ |
| 0.2 | Polished wood | 60-80 | 60+ |
| 0.3 | Ceramic glaze | 40-60 | 40+ |
| 0.4 | Chashu meat | 30-45 | 30+ |
| 0.5 | Noodles | 20-30 | 20+ |
| 0.6 | Raw wood | 10-20 | 10+ |
| 0.8 | Broth liquid | 5-15 | 5+ |
| 0.85 | Fabric/cloth | 2-5 | 2+ |
| 0.95 | Very matte | 1-2 | 1+ |

## Lighting Controls (Keyboard)

| Key | Action |
|-----|--------|
| `1` | Toggle Ambient Light |
| `2` | Toggle Diffuse Reflection |
| `3` | Toggle Specular Highlights |
| `4` | Toggle Directional Light (Moon) |
| `5` | Toggle Point Lights (Pendants/Lanterns) |
| `6` | Toggle Spot Light (Counter) |
| `7` | Toggle Area Light (Ceiling) |
| `P` | Cycle Lighting Presets |

## Light Presets Available

1. **All Lights On (Full Realism)** - All components enabled
2. **Spotlight Focus** - Only chef counter lighting
3. **Area Light Softbox** - Only ceiling diffuse
4. **Cozy Night** - Lanterns + moonlight
5. **Specular Highlights Only** - Just shininess
6. **Diffuse Shading Only** - No specularity
7. **Ambient Base Only** - Minimal fill light

## Color Temperature Reference

| Light Type | Color (RGB) | Kelvin | Use |
|------------|-----------|--------|-----|
| Tungsten Warm | (0.92, 0.68, 0.32) | 2700K | Interior pendant |
| Daylight Blue | (1.0, 0.96, 0.85) | 5000K | Task lighting |
| Moonlight Cool | (0.22, 0.22, 0.32) | 8000K | Directional fill |
| Lantern Warm | (0.88, 0.40, 0.08) | 2000K | Exterior glow |

## Adding New Materials

To create a custom material:

```cpp
// In shapes.h, add to Materials namespace:
namespace Materials {
	constexpr MaterialPBR MyMaterial = {
		0.15f,      // metallic (0=dielectric, 1=metal)
		0.35f,      // roughness (0=smooth, 1=matte)
		1.45f,      // IOR (index of refraction)
		{1.0f, 1.0f, 1.0f}  // base color (white)
	};
}

// In your drawing code:
setMaterialPBR(Materials::MyMaterial, surfaceColor);
drawYourObject();
```

## Metallic-Specific Material

```cpp
// For reflective metals, use:
setMaterialPBRMetallic(Materials::Polished, CHROME);
beginSphereReflect();
drawShinyObject();
endSphereReflect();
```

## Common Patterns

### Glossy Liquid (Broth, Water, etc.)
```cpp
setMaterialPBR(Materials::Broth, COLOR);
setMaterialGloss(0.90f, 0.75f, 0.50f, 85.0f);
drawLiquidSurface();
resetMaterialGloss();
```

### Matte Natural Material (Wood, Cloth, etc.)
```cpp
setMaterialPBR(Materials::WoodMatte, COLOR);
drawWoodObject();
resetMaterialGloss();  // Automatically handled by PBR
```

### Polished Metal with Reflection
```cpp
setMaterialPBRMetallic(Materials::Polished, METAL_COLOR);
beginSphereReflect();
drawMetalObject();
endSphereReflect();
resetMaterialGloss();
```

## Performance Tips

✅ **setMaterialPBR()** is fast - just sets GL material properties
✅ **beginSphereReflect()** / **endSphereReflect()** use early exit if no environment map
✅ **No shader compilation** - Fixed-function pipeline
✅ **Minimal state changes** - Group similar materials together

## Troubleshooting

**Problem**: Highlights too bright
- **Solution**: Reduce metallic value or increase roughness

**Problem**: Object looks flat
- **Solution**: Use glossy material (lower roughness, higher metallic)

**Problem**: Metal doesn't reflect environment
- **Solution**: Call beginSphereReflect() / endSphereReflect() around drawing code

**Problem**: Light is too bright/dim
- **Solution**: Adjust light intensity in scene.cpp GL_LIGHT config, or attenuation in main.cpp

**Problem**: Specular highlights jagged
- **Solution**: Increase shininess value (max 128), reduce roughness in material
