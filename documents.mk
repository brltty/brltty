clean-documents:
	$(MAKE) --directory $(DOCUMENTS_DIRECTORY) clean

documents: clean-documents
	$(MAKE) --directory $(DOCUMENTS_DIRECTORY) all

