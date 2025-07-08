#!/usr/bin/env python3
"""
T-Deck-Pro OS - Post-Build Script
Handles post-build tasks for the T-Deck-Pro operating system
"""

import os
import sys
import shutil
from pathlib import Path

# Add PlatformIO environment to Python path
try:
    Import("env")
except:
    print("Warning: Not running in PlatformIO environment")
    env = None

def print_build_summary():
    """Print build summary information"""
    print("=" * 60)
    print("T-DECK-PRO OS - POST-BUILD SCRIPT")
    print("=" * 60)
    
    if env:
        print(f"Environment: {env.get('PIOENV', 'Unknown')}")
        print(f"Platform: {env.get('PIOPLATFORM', 'Unknown')}")
        print(f"Framework: {env.get('PIOFRAMEWORK', 'Unknown')}")
    
    print("=" * 60)

def analyze_firmware_size():
    """Analyze and report firmware size"""
    print("Analyzing firmware size...")
    
    if not env:
        print("⚠ Environment not available - skipping size analysis")
        return
    
    # Get build directory
    build_dir = env.subst("$BUILD_DIR")
    firmware_path = Path(build_dir) / "firmware.bin"
    
    if firmware_path.exists():
        size = firmware_path.stat().st_size
        size_kb = size / 1024
        size_mb = size / (1024 * 1024)
        
        print(f"✓ Firmware size: {size:,} bytes ({size_kb:.1f} KB / {size_mb:.2f} MB)")
        
        # Check against ESP32-S3 flash size limits
        max_app_size = 6 * 1024 * 1024  # ~6MB typical max app partition
        if size > max_app_size:
            print(f"⚠ Warning: Firmware size exceeds typical app partition limit!")
            print(f"  Consider enabling optimization or reducing features")
        else:
            remaining = max_app_size - size
            remaining_mb = remaining / (1024 * 1024)
            print(f"✓ Remaining space: {remaining_mb:.2f} MB")
    else:
        print("⚠ Firmware binary not found")

def generate_build_artifacts():
    """Generate additional build artifacts"""
    print("Generating build artifacts...")
    
    if not env:
        print("⚠ Environment not available - skipping artifact generation")
        return
    
    # Create artifacts directory
    artifacts_dir = Path("build_artifacts")
    artifacts_dir.mkdir(exist_ok=True)
    
    # Get build directory
    build_dir = Path(env.subst("$BUILD_DIR"))
    env_name = env.get('PIOENV', 'unknown')
    
    # Copy firmware binary with timestamp
    firmware_src = build_dir / "firmware.bin"
    if firmware_src.exists():
        from datetime import datetime
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        firmware_dst = artifacts_dir / f"t-deck-pro-os_{env_name}_{timestamp}.bin"
        shutil.copy2(firmware_src, firmware_dst)
        print(f"✓ Firmware copied: {firmware_dst}")
    
    # Copy ELF file for debugging
    elf_src = build_dir / "firmware.elf"
    if elf_src.exists():
        elf_dst = artifacts_dir / f"t-deck-pro-os_{env_name}.elf"
        shutil.copy2(elf_src, elf_dst)
        print(f"✓ ELF file copied: {elf_dst}")
    
    # Generate memory map if available
    map_src = build_dir / "firmware.map"
    if map_src.exists():
        map_dst = artifacts_dir / f"t-deck-pro-os_{env_name}.map"
        shutil.copy2(map_src, map_dst)
        print(f"✓ Memory map copied: {map_dst}")

def create_ota_package():
    """Create OTA update package"""
    print("Creating OTA package...")
    
    if not env:
        print("⚠ Environment not available - skipping OTA package")
        return
    
    env_name = env.get('PIOENV', 'unknown')
    
    # Only create OTA packages for release builds
    if 'release' not in env_name and 'ota' not in env_name:
        print("⚠ Skipping OTA package (not a release/OTA build)")
        return
    
    # Create OTA directory
    ota_dir = Path("ota_packages")
    ota_dir.mkdir(exist_ok=True)
    
    # Get build directory and firmware
    build_dir = Path(env.subst("$BUILD_DIR"))
    firmware_src = build_dir / "firmware.bin"
    
    if not firmware_src.exists():
        print("✗ Firmware binary not found for OTA package")
        return
    
    # Create OTA package with version info
    from datetime import datetime
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # Read build info for version
    version = "1.0.0"  # Default version
    try:
        build_info_path = Path("src/core/build_info.h")
        if build_info_path.exists():
            with open(build_info_path, 'r') as f:
                content = f.read()
                for line in content.split('\n'):
                    if 'BUILD_VERSION' in line and '"' in line:
                        version = line.split('"')[1]
                        break
    except:
        pass
    
    # Copy firmware as OTA package
    ota_package = ota_dir / f"t-deck-pro-os_v{version}_{timestamp}.bin"
    shutil.copy2(firmware_src, ota_package)
    
    # Create version info file
    version_info = {
        "version": version,
        "timestamp": timestamp,
        "environment": env_name,
        "size": firmware_src.stat().st_size,
        "filename": ota_package.name
    }
    
    import json
    version_file = ota_dir / f"version_v{version}_{timestamp}.json"
    with open(version_file, 'w') as f:
        json.dump(version_info, f, indent=2)
    
    print(f"✓ OTA package created: {ota_package}")
    print(f"✓ Version info: {version_file}")

def cleanup_build_files():
    """Clean up temporary build files"""
    print("Cleaning up build files...")
    
    # Remove auto-generated build info (will be regenerated next build)
    build_info_path = Path("src/core/build_info.h")
    if build_info_path.exists():
        try:
            build_info_path.unlink()
            print("✓ Cleaned up build_info.h")
        except:
            print("⚠ Could not remove build_info.h")

def print_build_success():
    """Print build success message"""
    print("=" * 60)
    print("🚀 T-DECK-PRO OS BUILD COMPLETE!")
    print("=" * 60)
    print("Next steps:")
    print("1. Flash firmware to T-Deck-Pro device")
    print("2. Check serial monitor for boot messages")
    print("3. Test core functionality")
    print("4. Deploy to production if release build")
    print("=" * 60)

def main():
    """Main post-build function"""
    print_build_summary()
    analyze_firmware_size()
    generate_build_artifacts()
    create_ota_package()
    cleanup_build_files()
    print_build_success()

if __name__ == "__main__":
    main()