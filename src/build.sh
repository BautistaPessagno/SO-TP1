#!/bin/bash

# Build script for ChompChamps using Docker agodio/itba-so:2.0
# This script provides easy commands for building and testing inside Docker containers

set -e  # Exit on any error

# Docker configuration
DOCKER_IMAGE="agodio/itba-so:2.0"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Show usage
show_usage() {
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  all       - Pull Docker image and build all components (default)"
    echo "  build     - Build all components using Docker"
    echo "  master    - Build only master using Docker"
    echo "  view      - Build only view using Docker"
    echo "  players   - Build only players using Docker"
    echo "  player1   - Build only player1 (conservative) using Docker"
    echo "  player2   - Build only player2 (aggressive) using Docker"
    echo "  clean     - Clean all build artifacts"
    echo "  test      - Run compilation tests inside Docker"
    echo "  deps      - Check dependencies"
    echo "  debug     - Build with debug flags using Docker"
    echo "  release   - Build with optimization flags using Docker"
    echo "  install   - Build and install to bin/"
    echo "  run       - Run ChompChamps inside Docker container"
    echo "  structure - Show project structure"
    echo "  setup     - Pull Docker image"
    echo "  info      - Show build system information"
    echo "  help      - Show this help"
    echo ""
    echo "Docker Image: $DOCKER_IMAGE"
    echo ""
    echo "Examples:"
    echo "  $0                 # Pull image and build everything"
    echo "  $0 build          # Build using Docker"
    echo "  $0 run            # Run the game inside Docker"
    echo "  $0 test           # Test compilation inside Docker"
    echo "  $0 clean          # Clean build files"
}

# Check if we're in the right directory
check_directory() {
    if [[ ! -f "Makefile" ]]; then
        print_error "Makefile not found. Please run this script from the ChompChamps directory."
        exit 1
    fi
    
    if [[ ! -d "include" ]]; then
        print_error "include/ directory not found. Please run this script from the ChompChamps directory."
        exit 1
    fi
    
    if [[ ! -d "ipc" ]]; then
        print_error "ipc/ directory not found. Please run this script from the ChompChamps directory."
        exit 1
    fi
}

# Check system requirements
check_requirements() {
    print_status "Checking system requirements..."
    
    # Check for Docker
    if ! command -v docker &> /dev/null; then
        print_error "Docker not found. Please install Docker."
        exit 1
    fi
    
    # Check if Docker daemon is running
    if ! docker info &> /dev/null; then
        print_error "Docker daemon is not running. Please start Docker."
        exit 1
    fi
    
    # Check for make
    if ! command -v make &> /dev/null; then
        print_error "make not found. Please install make."
        exit 1
    fi
    
    print_success "System requirements check passed"
}

# Run the build
run_build() {
    local target=$1
    
    print_status "Building target: $target"
    
    if make $target; then
        print_success "Build completed successfully!"
        
        # Show what was built
        echo ""
        echo "Built executables:"
        ls -la master view player1 player2 2>/dev/null | grep -v "cannot access" || true
        
    else
        print_error "Build failed!"
        exit 1
    fi
}

# Main execution
main() {
    local command=${1:-all}
    
    case $command in
        all)
            check_directory
            check_requirements
            print_status "Pulling Docker image and building all components..."
            run_build "all"
            ;;
        build|docker-build)
            check_directory
            check_requirements
            print_status "Building using Docker..."
            run_build "docker-build"
            ;;
        master|view|players|player1|player2|system)
            check_directory
            check_requirements
            print_status "Building $command using Docker..."
            run_build "docker-build"
            ;;
        setup|docker-setup)
            check_directory
            check_requirements
            print_status "Setting up Docker environment..."
            run_build "docker-setup"
            ;;
        clean)
            check_directory
            print_status "Cleaning build artifacts..."
            make clean
            print_success "Clean completed!"
            ;;
        test)
            check_directory
            check_requirements
            print_status "Running compilation tests inside Docker..."
            make docker-test
            print_success "All tests passed!"
            ;;
        deps)
            check_directory
            print_status "Checking dependencies..."
            make deps
            print_success "Dependencies check completed!"
            ;;
        debug)
            check_directory
            check_requirements
            print_status "Building debug version using Docker..."
            make debug
            print_success "Debug build completed!"
            ;;
        release)
            check_directory
            check_requirements
            print_status "Building release version using Docker..."
            make release
            print_success "Release build completed!"
            ;;
        install)
            check_directory
            check_requirements
            print_status "Building and installing..."
            make install
            print_success "Installation completed!"
            ;;
        run)
            check_directory
            check_requirements
            print_status "Running ChompChamps inside Docker..."
            make run
            ;;
        structure)
            check_directory
            make structure
            ;;
        info)
            check_directory
            make info
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            print_error "Unknown command: $command"
            echo ""
            show_usage
            exit 1
            ;;
    esac
}

# Script information
echo "ChompChamps Build Script"
echo "========================"
echo "Docker Image: $DOCKER_IMAGE"
echo ""

# Run main function with all arguments
main "$@"
