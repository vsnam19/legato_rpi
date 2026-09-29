# ==============================================================================
# Qualcomm TelAF Simulation & Custom Vendor Monorepo Makefile
# ==============================================================================

.PHONY: help all test coverage build-app build-system deploy \
        run-sim stop-sim sim-status sim-shell \
        patch-apply patch-revert patch-status clean

help:
	@echo "====================================================================="
	@echo " Qualcomm TelAF Simulation & Vendor Monorepo CLI"
	@echo "====================================================================="
	@echo " Development Targets:"
	@echo "   make test          - Run Google Test C++20 unit tests (45 tests)"
	@echo "   make coverage      - Run unit tests with C2 branch coverage report"
	@echo "   make build-app     - Build standalone packages (cfgManager & cfgClient)"
	@echo "   make build-system  - Build integrated TelAF simulation system image"
	@echo "   make deploy        - Deploy update packages to simulation runtime"
	@echo ""
	@echo " Simulation Environment Targets:"
	@echo "   make run-sim       - Start simulation runtime container"
	@echo "   make stop-sim      - Stop simulation runtime container"
	@echo "   make sim-status    - Check simulation container status"
	@echo "   make sim-shell     - Open interactive shell in runtime container"
	@echo ""
	@echo " Upstream Patch Management:"
	@echo "   make patch-apply   - Apply all upstream patches"
	@echo "   make patch-revert  - Revert all upstream patches to pristine state"
	@echo "   make patch-status  - Display patch application status"
	@echo ""
	@echo " Maintenance:"
	@echo "   make clean         - Clean build directories and temporary files"
	@echo "====================================================================="

all: test build-app

test:
	@./scripts/run-unit-tests.sh

coverage:
	@./scripts/run-unit-tests.sh --coverage

build-app:
	@./scripts/build.sh --standalone

build-system:
	@./scripts/build.sh --integrated

deploy:
	@./scripts/deploy.sh

run-sim:
	@./scripts/run-simulation.sh start

stop-sim:
	@./scripts/run-simulation.sh stop

sim-status:
	@./scripts/run-simulation.sh status

sim-shell:
	@./scripts/run-simulation.sh shell

patch-apply:
	@./scripts/patch.sh apply

patch-revert:
	@./scripts/patch.sh revert

patch-status:
	@./scripts/patch.sh status

clean:
	@rm -rf build/
	@rm -rf vendor/custom/apps/cfgManager/_build_*
	@rm -rf vendor/custom/samples/cfgClient/_build_*
	@rm -rf vendor/custom/_build_*
	@rm -f vendor/custom/apps/cfgManager/*.update
	@rm -f vendor/custom/samples/cfgClient/*.update
	@find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
	@echo "Clean completed."
