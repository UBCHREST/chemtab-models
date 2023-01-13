ARG ABLATE_DEPENDENCY_IMAGE=ghcr.io/ubchrest/ablate/ablate-dependencies-gcc:latest
FROM $ABLATE_DEPENDENCY_IMAGE

# Copy over the source
COPY . /source-chemtab
WORKDIR /build-chemtab

# Configure & build
run cmake -DCMAKE_BUILD_TYPE=Release -S /source-chemtab/tests -B .
run make -j $(nproc)

# Specify Entry Point for tests
ENV CTEST_OUTPUT_ON_FAILURE=ON
CMD bash -c "echo 'Running Tests for ChemTab' && ctest "