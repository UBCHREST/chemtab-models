ARG ABLATE_DEPENDENCY_IMAGE=ghcr.io/ubchrest/ablate/ablate-dependencies-gcc:latest
FROM $ABLATE_DEPENDENCY_IMAGE

# Copy over the source
COPY . /source-chemtab
WORKDIR /build-chemtab

# Setup the petsc env for tests
ARG PETSC_BUILD_ARCH=arch-ablate-opt
ENV PETSC_ARCH=$PETSC_BUILD_ARCH
ENV PKG_CONFIG_PATH="${PETSC_DIR}/${PETSC_ARCH}/lib/pkgconfig:$PKG_CONFIG_PATH"
ENV PATH="${PETSC_DIR}/${PETSC_ARCH}/bin:$PATH"

# Configure & build
RUN cmake -DCMAKE_BUILD_TYPE=Release -S /source-chemtab/tests -B .
RUN make -j $(nproc)

# Specify Entry Point for tests
ENV CTEST_OUTPUT_ON_FAILURE=ON
CMD bash -c "echo 'Running Tests for ChemTab' && make format-check-chemtab && ctest -L 'chemTabModelTests' "