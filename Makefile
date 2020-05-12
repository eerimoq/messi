test:
	python3 setup.py test
	$(MAKE) -C tst

release-to-pypi:
	python3 setup.py sdist
	python3 setup.py bdist_wheel
	twine upload dist/*
