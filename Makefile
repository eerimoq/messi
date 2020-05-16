test:
	python3 setup.py test
	$(MAKE) -C tst

test-all: test test-sdist
	$(MAKE) -C examples

test-sdist:
	rm -rf dist
	python3 setup.py sdist
	cd dist && \
	mkdir test && \
	cd test && \
	tar xf ../*.tar.gz && \
	cd messi-* && \
	python3 setup.py test

release-to-pypi:
	python3 setup.py sdist
	python3 setup.py bdist_wheel
	twine upload dist/*
